# ***************************************
# Author: Jing Xie
# Created Date: 2020-5-1
# Updated Date: 2020-6-1
# Email: jing.xie@pnnl.gov
# ***************************************

import pandas as pd
# import seaborn as sns
import matplotlib.pyplot as plt

import re
import os.path
import sys
import math


class CsvExt:
    """Extract data from (.csv) files of results"""

    def __init__(self, csv_folder_path, csv_file_name):
        """Init the settings
        """
        # ==CSV
        self.csv_folder_path = csv_folder_path
        self.csv_file_name = csv_file_name

        # ==DataFrame
        self.csv_df = None
        self.nd_volt_v_df = None
        self.nd_volt_mag_v_df = None
        self.nd_delta_volt_mag_v_df = None
        
        self.swt_dv_df = None
        
        self.pkg_df = None

        # ==Series
        self.pv_q_pu_ser = None
        self.pv_q_var_ser = None
        self.pv_price_dollar_ser = None
        
        # ==Others
        self.pv_rating = 0

        # ==Preprocess
        self.csv_pfn = os.path.join(csv_folder_path, csv_file_name)

    def read_csv(
        self,
        csv_pfn="",
        skiprows_list=[x for x in range(7)],
        skipinitialspace_flag=True,
    ):
        """Get the DataFrame
        """
        if not csv_pfn:
            csv_pfn = self.csv_pfn

        self.csv_df = pd.read_csv(
            csv_pfn, skiprows=skiprows_list, skipinitialspace=skipinitialspace_flag
        )

    def pre_process(self, num_nds=8, num_phs=3):
        """Get the Selected Data Columns
        """
        #== Q
        self.pv_q_pu_ser = self.csv_df.iloc[:, -1]
        self.pv_q_var_ser = self.csv_df.iloc[:, 1]
        
        ind_q_zero = pd.Index(self.pv_q_pu_ser).get_loc(0)
        
        #== V & Delta V
        self.nd_volt_v_df = self.csv_df.iloc[:, 2 : 2 + num_nds * num_phs]
        self.nd_volt_mag_v_df = pd.DataFrame()
        self.nd_delta_volt_mag_v_df = pd.DataFrame()
        for cur_col in self.nd_volt_v_df.columns:
            self.nd_volt_v_df[cur_col] = self.nd_volt_v_df[cur_col].apply(lambda x: CsvExt.parse_volt_phasor(x))
            self.nd_volt_mag_v_df[cur_col] = self.nd_volt_v_df[cur_col].apply(lambda x: abs(x))
            self.nd_delta_volt_mag_v_df[cur_col] = self.nd_volt_mag_v_df[cur_col].apply(lambda x: x - self.nd_volt_mag_v_df.loc[ind_q_zero,cur_col])
        
    def plot_dq_dv(self, filter_flag=True, ls_th=1, fig_fmt_str='.svg', fmt_dic = {'fontname':'Times New Roman','size': 16}):        
        if filter_flag:            
            nd_delta_volt_mag_v_df_ff = self.nd_delta_volt_mag_v_df.loc[:, self.nd_delta_volt_mag_v_df.nunique() > ls_th]
        else:
            nd_delta_volt_mag_v_df_ff = self.nd_delta_volt_mag_v_df.copy()
            
        plt.plot(self.pv_q_var_ser, nd_delta_volt_mag_v_df_ff)
        
        plt.title(self.csv_file_name, **fmt_dic)
        plt.xlabel(r"Delta Q (var)", **fmt_dic)
        plt.ylabel(r"Delta V (V)", **fmt_dic)
        plt.grid()
        lgd = plt.legend(list(nd_delta_volt_mag_v_df_ff.columns),
                   loc='upper left',bbox_to_anchor= (0.0, -0.22),
                   ncol=2)
        
        fig_fn = os.path.splitext(self.csv_file_name)[0] + fig_fmt_str
        fig_fpn = os.path.join(self.csv_folder_path, fig_fn)
        plt.savefig(fig_fpn,
                    bbox_extra_artists=(lgd,),bbox_inches='tight')
        plt.show()

    def eval_dq_dv(self):
        self.read_csv()
        self.pre_process()
        self.plot_dq_dv()

    def get_price_q(self, coef_rt = 0.1/1e3):
        """
        pv_price_dollar_per_h = coef_rt*(sqrt(pv_rating^2-pv_q_init^2) - pv_p_init) 
                                    - coef_rt*(sqrt(pv_rating^2-(pv_q_init+pv_q)^2) - pv_p_init)
        which can be simplified as follows, when pv_p_init = pv_q_init = 0
        pv_price_dollar_per_h = coef_rt*(pv_rating - sqrt(pv_rating^2-pv_q^2))
        1) price is with the unit of $/h
        2) coef_rt is with the unit of $/Wh
        """
        ind_q_one = pd.Index(self.pv_q_pu_ser).get_loc(1)
        self.pv_rating = self.pv_q_var_ser[ind_q_one]        
        
        self.pv_price_dollar_ser = coef_rt * (self.pv_rating - 
                        (self.pv_rating**2 - self.pv_q_var_ser.pow(2)).pow(0.5))
        

    def plot_price_q_curve(self, saved_fig_pref_str = 'price_', fig_fmt_str='.svg', fmt_dic = {'fontname':'Times New Roman','size': 16}):
        plt.plot(self.pv_q_var_ser/1e3, self.pv_price_dollar_ser)   
        
        plt.title(self.csv_file_name, **fmt_dic)
        plt.xlabel(r"Delta Q (kVar)", **fmt_dic)
        plt.ylabel(r"Price ($/h)", **fmt_dic)
        
        plt.grid()
        # plt.ticklabel_format(style='sci', axis='x', scilimits=(0,0))
        
        ax = plt.gca()
        for tick in ax.xaxis.get_major_ticks():
            tick.label1.set_fontweight('bold')
        for tick in ax.yaxis.get_major_ticks():
            tick.label1.set_fontweight('bold')
        
        fig_fn = saved_fig_pref_str + os.path.splitext(self.csv_file_name)[0] + fig_fmt_str
        fig_fpn = os.path.join(self.csv_folder_path, fig_fn)
        plt.savefig(fig_fpn)
        plt.show()
        
    # def plot_other_prices(self, dt_h = 1):
    #     pv_price_dollar_by_dt_ser = self.pv_price_dollar_ser * dt_h
    #     pv_price_dpvar = pv_price_dollar_by_dt_ser/abs(self.pv_q_var_ser)
        

    def eval_price_q(self):
        self.read_csv()
        self.pre_process()
        self.get_price_q()
        self.plot_price_q_curve()

    def get_dv_swt(self, dict_swt, list_ph_str=['voltage_A','voltage_B','voltage_C'], delim_str=':'):
        if dict_swt == None:
            return
        self.swt_dv_df = pd.DataFrame()
        for cur_key, cur_list in dict_swt.items():
            for cur_ph in list_ph_str:
                cur_col_suff_str = delim_str + cur_ph
                # self.swt_dv_df[cur_key+cur_col_suff_str] = abs(self.nd_volt_mag_v_df[cur_list[0]+cur_col_suff_str]
                #                - self.nd_volt_mag_v_df[cur_list[1]+cur_col_suff_str])
                self.swt_dv_df[cur_key+cur_col_suff_str] = (self.nd_volt_mag_v_df[cur_list[0]+cur_col_suff_str]
                               - self.nd_volt_mag_v_df[cur_list[1]+cur_col_suff_str])
                
                #@TODO: swt_dv_df is only the dv of two nodes. It may need to be the dv from the 0 q injection (ref: self.nd_delta_volt_mag_v_df[cur_col] = self.nd_volt_mag_v_df[cur_col].apply(lambda x: x - self.nd_volt_mag_v_df.loc[ind_q_zero,cur_col]))
    
    def filter_line_slope(self, ls_th=1):
        self.swt_dv_df = self.swt_dv_df.loc[:, self.swt_dv_df.nunique() > ls_th]

    def plot_dv_swt(self, fig_pref_str = 'swt_', fig_fmt_str = '.svg', fmt_dic = {'fontname':'Times New Roman','size': 16}):
        plt.plot(self.pv_q_var_ser, self.swt_dv_df)
        
        plt.title(self.csv_file_name, **fmt_dic)
        plt.xlabel(r"Delta Q (var)", **fmt_dic)
        plt.ylabel(r"Delta V (V)", **fmt_dic)
        
        plt.grid()
        lgd = plt.legend(list(self.swt_dv_df.columns),
                    loc='upper left', bbox_to_anchor= (0.0, -0.22),
                    ncol=3)
        
        fig_fn = fig_pref_str + os.path.splitext(self.csv_file_name)[0] + fig_fmt_str
        fig_fpn = os.path.join(self.csv_folder_path, fig_fn)
        plt.savefig(fig_fpn,
                    bbox_extra_artists=(lgd,),bbox_inches='tight')
        plt.show()
    
    def eval_swt_dv(self, dict_swt, flag_filter=True):
        self.read_csv()
        self.pre_process()
        self.get_dv_swt(dict_swt)
        if flag_filter:
            self.filter_line_slope()
        self.plot_dv_swt()
        
    def package_df(self):
        self.pv_q_var_ser.name = "q_var"
        self.pv_price_dollar_ser.name = "price_dph"
        cct_list = [self.pv_q_var_ser, self.pv_price_dollar_ser, self.nd_delta_volt_mag_v_df, self.swt_dv_df]
        self.pkg_df = pd.concat(cct_list, axis = 1)
        
    def eval_package_df(self, dict_swt = None):
        self.read_csv()
        self.pre_process()
        self.get_dv_swt(dict_swt)
        self.get_price_q()
        self.package_df()
    
    @staticmethod
    def parse_volt_phasor(volt_ph_str):
        def get_float_list(ori_str):
            float_reg = re.compile(r"[-+]?\d+\.?\d*")
            return float_reg.findall(ori_str)

        try:
            if volt_ph_str[-1] == 'j':
                return complex(volt_ph_str)
            elif volt_ph_str[-1] == 'd':
                cur_float_list = get_float_list(volt_ph_str)
                if len(cur_float_list) != 2:
                    raise Exception("Voltage phasor string ends with 'd' but is incorrect.")
                else:
                    cur_volt_mag = float(cur_float_list[0])
                    cur_volt_ang_rad = math.radians(float(cur_float_list[1]))
                    return complex(cur_volt_mag*math.cos(cur_volt_ang_rad), cur_volt_mag*math.sin(cur_volt_ang_rad))
            else:
                raise Exception("Voltage phasor string ends with either 'j' or 'd'.")
        except IndexError as err:
            print(f"The input string cannot be empty: {err}")
        except Exception as err:
            print(f"The input string has an unknown format: {err}")
        except:
            print("Unexpected error:", sys.exc_info()[0])
            raise

def test_parse_volt_phasor():
    # aa = "+2+22j-2"
    bb = "-10.0-60d"
    
    # float_reg = re.compile(r"[-+]?\d+\.?\d*")
    # cc = float_reg.findall(bb)

    cc = CsvExt.parse_volt_phasor(bb)
    print(cc)

def test_CsvExt():
    """
    Params & Init
    """
    # ==Parameters (.csv files)
    # csv_folder_path = r"D:\csv files_all"
    csv_folder_path = r"D:\csv files_UC1SC1_InitTopo"
    
    # csv_file_name = r"Inv_S6_n865810391_1209.csv"
    csv_file_name = r"Inv_S10_n865809671_1212.csv"
    # csv_file_name = r"Inv_S1_n256851477_1207.csv"

    # ==Create an Instance of CsvExt
    p = CsvExt(csv_folder_path, csv_file_name)

    """
    Demo 01 (extract & plot deltaQ-deltaV curve)
    """
    # ==Option 0 (run all .csv files in a given folder)
    # import glob
    
    # csv_fpn_list = glob.glob(os.path.join(csv_folder_path, '*.csv'))
    # for cur_csv_fpn in csv_fpn_list:
    #     # _, cur_csv_fn = os.path.split(cur_csv_fpn)
    #     cur_p = CsvExt(csv_folder_path, os.path.basename(cur_csv_fpn))
    #     cur_p.eval_dq_dv()

    # ==Option 1
    # p.eval_dq_dv()
    
    # ==Option 2
    # p.read_csv()
    # p.pre_process()
    # p.plot_dq_dv()

    """
    Demo 02 (get the price-Q curve)
    """
    # ==Option 0
    # import glob
    
    # csv_fpn_list = glob.glob(os.path.join(csv_folder_path, '*.csv'))
    # for cur_csv_fpn in csv_fpn_list:
    #     # _, cur_csv_fn = os.path.split(cur_csv_fpn)
    #     cur_p = CsvExt(csv_folder_path, os.path.basename(cur_csv_fpn))
    #     cur_p.eval_price_q()
    
    # ==Option 1
    p.eval_price_q()
    
    # ==Option 2
    # p.read_csv()
    # p.pre_process()
    # p.get_price_q()
    # p.plot_price_q_curve()

    """
    Demo 03 (deltaV of a switch)
    """    
    # ==Param
    dict_swt = {
        "RCL2": ["n264462735_1209", "n256860543_1207"],
        "RCL7": ["n259333341_1212", "n617197553_1209"],
        "RCL9": ["n439934984_1210", "n256904390_1209"],
        "RCL11": ["n256834423_1212", "n616009828_1210"]
    }
    
    # ==Option 0 (run all .csv files in a given folder)
    # import glob
    
    # csv_fpn_list = glob.glob(os.path.join(csv_folder_path, '*.csv'))
    # for cur_csv_fpn in csv_fpn_list:
    #     cur_p = CsvExt(csv_folder_path, os.path.basename(cur_csv_fpn))
    #     cur_p.eval_swt_dv(dict_swt)    

    # ==Option 1
    # p.eval_swt_dv(dict_swt)
    
    # ==Option 2
    # p.read_csv()
    # p.pre_process()
    # p.get_dv_swt(dict_swt)
    # p.filter_line_slope()
    # p.plot_dv_swt()
   
    """
    Demo 03 (package dataframe)
    """    
    # ==Param
    dict_swt = {
        "RCL2": ["n264462735_1209", "n256860543_1207"],
        "RCL7": ["n259333341_1212", "n617197553_1209"],
        "RCL9": ["n439934984_1210", "n256904390_1209"],
        "RCL11": ["n256834423_1212", "n616009828_1210"]
    }
    
    pickle_fp = csv_folder_path
    pickle_fn = r"all_pv_df.pickle"
    
    # ==Option 0
    import glob
    
    pv_df_list = {}
    
    csv_fpn_list = glob.glob(os.path.join(csv_folder_path, '*.csv'))
    for cur_csv_fpn in csv_fpn_list:
        cur_p = CsvExt(csv_folder_path, os.path.basename(cur_csv_fpn))
        cur_p.eval_package_df(dict_swt)
        
        cur_fn_nsfx = os.path.splitext(os.path.basename(cur_csv_fpn))[0]
        pv_df_list[cur_fn_nsfx] = cur_p.pkg_df
        
    all_pv_df = pd.concat(pv_df_list)
    
    # all_pv_df.to_csv("validate.csv")
    # print(all_pv_df)
    
    # --Save
    import pickle
    
    pickle_fpn = os.path.join(pickle_fp, pickle_fn)
    
    hf_pkl_all_pv_df = open(pickle_fpn, "wb")
    pickle.dump(all_pv_df, hf_pkl_all_pv_df)
    hf_pkl_all_pv_df.close()
    
    # ==Option 1
    # p.eval_package_df(dict_swt)
    # p.eval_package_df()
    

if __name__ == "__main__":
    test_CsvExt()

    # test_parse_volt_phasor()
