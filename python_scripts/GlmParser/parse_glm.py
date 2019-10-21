## ***************************************
# Author: Jing Xie
# Updated Date: 2019-10-15
# Email: jing.xie@pnnl.gov
#  ***************************************

import re
import os.path


class GlmParser:
    """Parse the .glm file(s) and analysis/export"""
    def __init__(self, master_file=''):
        #==Reserve for handling '#include'
        self.master_file = os.path.basename(master_file)
        self.file_path = os.path.dirname(master_file)
        self.master_file_noext, _ = os.path.splitext(master_file)

        #==Store
        self.all_loads_list = []
        self.all_loads_p_list = []

        #==GLM SYN
        self.re_glm_syn_comm = '\/\/.*\n';

        #==GLM OBJ TPL
        self.obj_load_tpl_str = "object load {{\
    {}\
    {}\
    }}\n"
        self.obj_triload_tpl_str = "object triplex_load {{\
    {}\
    {}\
    }}\n"
        self.sobj_gfa_tpl_str = "\n\
    //--GFA--\n\
    frequency_measure_type PLL;\n\
    GFA_enable true;\n\
    GFA_freq_low_trip {} Hz;\n\
    GFA_reconnect_time {} s;\n\
    GFA_freq_disconnect_time {} s;\n\
    {}\n"

    def clean_buffer(self):
        self.all_loads_list = []
        self.all_loads_p_list = []

    def parse_load(self, lines_str):
        """Parse and Package All Load Objects
        """
        self.clean_buffer()
        self.all_loads_list = re.findall(r'object\s*load.*?{(.*?)}',lines_str,flags=re.DOTALL)

        for cur_obj_str in self.all_loads_list:
            cur_obj_s_list = re.findall(r'.*constant_power_A\s*(.*?);',cur_obj_str,flags=re.DOTALL)
            cur_obj_s_list += re.findall(r'.*constant_power_B\s*(.*?);',cur_obj_str,flags=re.DOTALL)
            cur_obj_s_list += re.findall(r'.*constant_power_C\s*(.*?);',cur_obj_str,flags=re.DOTALL)

            cur_obj_p_sum = 0
            for cur_ph_s_str in cur_obj_s_list:
                cur_obj_p_sum += complex(cur_ph_s_str).real

            self.all_loads_p_list.append(cur_obj_p_sum)

    def parse_triload(self, lines_str):
        """Parse and Package All Load Objects
        """
        self.clean_buffer()
        self.all_loads_list = re.findall(r'object\s*triplex_load.*?{(.*?)}',lines_str,flags=re.DOTALL)

        for cur_obj_str in self.all_loads_list:
            cur_obj_s_list = re.findall(r'.*constant_power_12\s*(.*?);',cur_obj_str,flags=re.DOTALL)

            cur_obj_p_sum = 0
            for cur_ph_s_str in cur_obj_s_list:
                cur_obj_p_sum += complex(cur_ph_s_str).real

            self.all_loads_p_list.append(cur_obj_p_sum)

    def import_file(self, filename):
        o_file = open(filename,'r')
        str_file = o_file.read()
        o_file.close()
        
        str_file_woc = re.sub(self.re_glm_syn_comm,'',str_file)
        return str_file_woc
        
    def read_content_load(self, filename):
        """This func is added as an extra layer for flexible extension"""
        str_file_woc = self.import_file(filename);
        self.parse_load(str_file_woc)

    def read_content_triload(self, filename):
        str_file_woc = self.import_file(filename);
        self.parse_triload(str_file_woc)

    def add_ufls_gfas(self, output_glm_path_fn,
                      ufls_pct,ufls_th,ufls_dly,gfa_rc_time,gfa_extra_str='',
                      flag_triload=False, flag_des=True):
        """ flag_des is reserved for extension
        """
        sorted_ind_list = sorted(range(len(self.all_loads_p_list)),
                             key=lambda k:self.all_loads_p_list[k],
                             reverse=flag_des)

        total_loads_p = sum(self.all_loads_p_list)
        ufls_p = [x/100*total_loads_p for x in ufls_pct];
        ufls_p_asg = [0]*len(ufls_p)
        ufls_gfa_num = [0]*len(ufls_p)
        all_loads_ufls_tag = [-1]*len(self.all_loads_p_list)
        
        for cur_ite in range(len(ufls_p)):
            for cur_ite_ld in range(len(sorted_ind_list)):
                cur_sorted_ind = sorted_ind_list[cur_ite_ld]
                if all_loads_ufls_tag[cur_sorted_ind] >= 0:
                    continue
                cur_load_p = self.all_loads_p_list[cur_sorted_ind]
                if cur_load_p == 0:
                    continue
                if (ufls_p_asg[cur_ite]+cur_load_p) <= ufls_p[cur_ite]:
                    ufls_p_asg[cur_ite] += cur_load_p
                    ufls_gfa_num[cur_ite] += 1
                    all_loads_ufls_tag[cur_sorted_ind] = cur_ite

        #==Display & Export
        if flag_triload:
            print('\n[Triplex Loads:]')
        else:
            print('\n[Loads:]')
        print('Total Load in kW: {}'.format(total_loads_p/1000))
        print('Assigned UFLS in kW: {}'.format([x/1000 for x in ufls_p_asg]))
        print('Assigned UFLS in %: {}'.format([x/total_loads_p*100 for x in ufls_p_asg]))
        print('Number of GFA devices: {}'.format(ufls_gfa_num))
        
        self.export_glm_with_gfas(output_glm_path_fn,all_loads_ufls_tag,gfa_extra_str,flag_triload)

    def prepare_export_file(self,file_pn):
        if os.path.exists(file_pn):
            os.remove(file_pn)
            print("The old '{}' file is deleted!".format(file_pn))

        hf_output = open(file_pn,'w')
        print("The new '{}' file is created!".format(file_pn))
        return hf_output

    def export_glm_with_gfas(self, output_glm_path_fn, all_loads_ufls_tag,gfa_extra_str,flag_triload):
        hf_output = self.prepare_export_file(output_glm_path_fn)
        for ld_str, gp in zip(self.all_loads_list,all_loads_ufls_tag):
            #print(ld_str)
            if gp >= 0:
                cur_gfa_str = self.sobj_gfa_tpl_str.format(ufls_th[gp],
                                                      gfa_rc_time,
                                                      ufls_dly[gp],
                                                      gfa_extra_str)
            else:
                cur_gfa_str = ''

            if flag_triload:
                hf_output.write(self.obj_triload_tpl_str.format(ld_str,cur_gfa_str))
            else:
                hf_output.write(self.obj_load_tpl_str.format(ld_str,cur_gfa_str))
        hf_output.close()





if __name__ == '__main__':
    #==Parameters
    glm_file_path_fn = 'loads.glm' #Note that the filename cannot end with slash(es)
    output_glm_path_fn = 'loads_gfa.glm'

    ufls_pct = [10, 15, 10] #Unit: %
    ufls_th = [59.0, 58.5, 58.0] #Unit: Hz
    ufls_dly = [0.1, 0.05, 0.08] #Unit: sec

    gfa_rc_time = 1000 #Unit: sec #Set this to a large value to avoid reconnection.
    gfa_extra_str = "\n\
    GFA_freq_high_trip 70 Hz;\n\
    GFA_volt_low_trip 0.5;\n\
    GFA_volt_high_trip 1.5;\n\
    GFA_volt_disconnect_time 0.08 s;"

    #==Test & Demo
    p = GlmParser()
    
    p.read_content_load(glm_file_path_fn)
    p.add_ufls_gfas(output_glm_path_fn,ufls_pct,ufls_th,ufls_dly,gfa_rc_time,gfa_extra_str)

    """
    p.read_content_triload('triplex_loads.glm')
    triload_flag = True
    p.add_ufls_gfas('triplex_loads_gfa.glm',ufls_pct,ufls_th,ufls_dly,gfa_rc_time,triload_flag)
    """
    
