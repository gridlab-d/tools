# ***************************************
# Author: Jing Xie
# Created Date: 2020-5-1
# Updated Date: 2020-5-25
# Email: jing.xie@pnnl.gov
# ***************************************

import pandas as pd
import seaborn as sns
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

        # ==Series
        self.pv_q_pu_ser = None
        self.pv_q_var_ser = None
        self.pv_price_dollar_ser = None

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
        
    def plot_dq_dv(self, fig_fmt_str='.svg', fmt_dic = {'fontname':'Times New Roman','size': 16}):
        # plt.figure(figsize=(16,6))
        plt.plot(self.pv_q_var_ser, self.nd_delta_volt_mag_v_df)        
        
        plt.title(self.csv_file_name, **fmt_dic)
        plt.xlabel(r"Delta Q (var)", **fmt_dic)
        plt.ylabel(r"Delta V (V)", **fmt_dic)
        plt.grid()
        lgd = plt.legend(list(self.nd_delta_volt_mag_v_df.columns),
                   loc='upper left',bbox_to_anchor= (0.0, -0.22),
                   ncol=2)
        
        fig_fn = os.path.splitext(self.csv_file_name)[0] + fig_fmt_str
        fig_fpn = os.path.join(self.csv_folder_path, fig_fn)
        plt.savefig(fig_fpn,
                    bbox_extra_artists=(lgd,),bbox_inches='tight')
        plt.show()

    def read_prop_plot(self):
        self.read_csv()
        self.pre_process()
        self.plot_dq_dv()

    def get_price_q_curve(self, coef_rt = 0.1, plot_flag = False, saved_fig_pref_str = 'p_q_', fig_fmt_str='.svg', fmt_dic = {'fontname':'Times New Roman','size': 16}):
        """
        Supply curve equation: Price = C_RT * [ (S_Inv^2 – Q^2) – ( S_Inv^2 – (Q+Q_Ofr)^2 ) ]
        where S_Inv is the rating of the inverter, Q = initial operating point (Q) of the inverter that is taken as 0 for now,
        and Q_Ofr = offered reactive power which is basically the one you vary. For now, please C_RT = 0.1 . This equation should give Price-reactive curve for the inverter which should be converted to voltage-reactive power using the sensitivity you obtained.
        """
        self.pv_price_dollar_ser = coef_rt * self.pv_q_var_ser.pow(2)

        if plot_flag:
            plt.plot(self.pv_q_var_ser, self.pv_price_dollar_ser)        
            
            plt.title(self.csv_file_name, **fmt_dic)
            plt.xlabel(r"Delta Q (var)", **fmt_dic)
            plt.ylabel(r"Price ($/var)", **fmt_dic)
            
            plt.grid()
            plt.ticklabel_format(style='sci', axis='x', scilimits=(0,0))
            
            ax = plt.gca()
            for tick in ax.xaxis.get_major_ticks():
                tick.label1.set_fontweight('bold')
            for tick in ax.yaxis.get_major_ticks():
                tick.label1.set_fontweight('bold')
            
            fig_fn = saved_fig_pref_str + os.path.splitext(self.csv_file_name)[0] + fig_fmt_str
            fig_fpn = os.path.join(self.csv_folder_path, fig_fn)
            plt.savefig(fig_fpn)
            plt.show()

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
    aa = "+2+22j-2"
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
    csv_folder_path = r"D:\csv files_all"
    csv_file_name = r"Inv_S1_n256851477_1207.csv"

    # ==Create an Instance of CsvExt
    p = CsvExt(csv_folder_path, csv_file_name)

    """
    Demos
    """
    # ==Demo 01 (extract & plot deltaQ-deltaV curve)
    # p.read_csv()
    # p.pre_process()

    # p.plot_dq_dv()

    # ==Demo 02 (run all .csv files in a given folder)
    # import glob
    
    # csv_fpn_list = glob.glob(os.path.join(csv_folder_path, '*.csv'))
    # for cur_csv_fpn in csv_fpn_list:
    #     # _, cur_csv_fn = os.path.split(cur_csv_fpn)
    #     cur_p = CsvExt(csv_folder_path, os.path.basename(cur_csv_fpn))
    #     cur_p.read_prop_plot()

    # ==Demo 03 (get the price-Q curve)
    p.read_csv()
    p.pre_process()
    p.get_price_q_curve(plot_flag = True)
        

if __name__ == "__main__":
    test_CsvExt()

    # test_parse_volt_phasor()
