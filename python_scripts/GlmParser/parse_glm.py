# ***************************************
# Author: Jing Xie
# Created Date: 2019-10
# Updated Date: 2020-11-25
# Email: jing.xie@pnnl.gov
# ***************************************

import re
import os.path
import csv
import math

import random

import pickle


class GlmParser:
    """Parse the .glm file(s) and analysis/export"""

    def __init__(self, master_file=""):
        # ==Reserve for handling '#include'
        self.master_file = os.path.basename(master_file)
        self.file_path = os.path.dirname(master_file)
        self.master_file_noext, _ = os.path.splitext(master_file)

        # ==Store
        # -- load
        self.all_loads_list = []
        self.all_loads_p_list = []
        self.all_loads_q_list = []

        # -- node
        self.all_nodes_list = []
        self.all_nodes_names_list = []
        self.all_nodes_phases_dict = {}

        self.all_nodes_3ph_list = []
        self.all_nodes_names_3ph_list = []
        self.all_nodes_phases_3ph_dict = []

        # -- triplex node
        self.all_triplex_nodes_list = []
        self.all_adj_triplex_nodes_list = None

        self.all_triplex_nodes_p_list = []
        self.all_triplex_nodes_q_list = []

        self.all_triplex_nodes_p1_list = []
        self.all_triplex_nodes_q1_list = []
        self.all_triplex_nodes_p2_list = []
        self.all_triplex_nodes_q2_list = []

        # ==GLM SYN
        self.re_glm_syn_comm = "\/\/.*\n"
        self.re_glm_syn_mty_lns = r"^(?:[\t ]*(?:\r?\n|\r))+"

        self.re_glm_syn_obj_load = r"object\s*load.*?{.*?}\s*;*"
        self.re_glm_syn_obj_node = r"object\s*node.*?{.*?}\s*;*"
        self.re_glm_syn_obj_inv = r"object\s*inverter.*?{.*?}\s*;*"

        self.re_glm_attr_tpl_str = ".*{}\s*(.*?);"

        # ==GLM OBJ TPL for EXPORT
        self.obj_tpl_str = "object {} {{\n" \
                           "{} {}" \
                           "}}\n"

        self.obj_load_tpl_str = "object load {{\
    {}\
    {}\
    }}\n"

        self.obj_triload_tpl_str = "object triplex_load {{\
    {}\
    {}\
    }}\n"

        self.obj_triplex_node_tpl_str = "object triplex_node {{\
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

    def clean_load_buffer(self):
        self.all_loads_list = []
        self.all_loads_p_list = []
        self.all_loads_q_list = []

    def clean_triplex_node_buffer(self):
        self.all_triplex_nodes_list = []
        self.all_adj_triplex_nodes_list = None

        self.all_triplex_nodes_p_list = []
        self.all_triplex_nodes_q_list = []

        self.all_triplex_nodes_p1_list = []
        self.all_triplex_nodes_q1_list = []
        self.all_triplex_nodes_p2_list = []
        self.all_triplex_nodes_q2_list = []

    def extract_attr(self, attr_str, src_str):
        """Extract the attribute information"""
        attr_list = re.findall(
            self.re_glm_attr_tpl_str.format(attr_str), src_str, flags=re.DOTALL
        )
        return attr_list

    def extract_obj(self, obj_str, src_str):
        """Extract the content of a giving type of object"""
        obj_list = re.findall(obj_str, src_str, flags=re.DOTALL)
        return obj_list

    def find_obj_via_attr(self, obj_str, attr_tag_str, attr_val_str, src_str):
        """ Find the content of an object using its type & attribute
        """
        re_tpl_obj_attr = (
                r"object\s*"
                + obj_str
                # + r"\s*{.*?"
                + r"\s*{[^}]*?"
                + attr_tag_str
                + r"\s*"
                + attr_val_str
                + r"\s*;.*?}"
        )

        extr_obj_list = self.extract_obj(re_tpl_obj_attr, src_str)
        return extr_obj_list, re_tpl_obj_attr

    def modify_attr(self, attr_tag_str, attr_val_str, cur_inv_glm_lines_str):
        cur_inv_glm_lines_mod_str = re.sub(
            r".*?" + attr_tag_str + r".*?;.*?",
            "\t" + attr_tag_str + " %s;" % attr_val_str,
            cur_inv_glm_lines_str,
        )
        return cur_inv_glm_lines_mod_str

    def replace_obj(self, re_tpl, src_str, rep_str):
        cur_q_inv_glm_str = re.sub(
            re_tpl,
            rep_str,
            src_str,
            flags=re.DOTALL,
        )
        return cur_q_inv_glm_str

    def parse_inv(self, lines_str):
        """Parse and Package All Inverter Objects
        """
        self.all_invs_list = self.extract_obj(self.re_glm_syn_obj_inv, lines_str)

        self.all_invs_names_list = []
        for cur_obj_str in self.all_invs_list:
            # ==Names
            cur_inv_obj_name_list = self.extract_attr("name", cur_obj_str)
            assert (
                    len(cur_inv_obj_name_list) == 1
            ), "Redundancy or missing on the name attribute!"
            self.all_invs_names_list.append(cur_inv_obj_name_list[0])

    def parse_node(self, lines_str):
        """Parse and Package All Node Objects
        """
        self.all_nodes_list = self.extract_obj(self.re_glm_syn_obj_node, lines_str)

        self.all_nodes_names_list = []
        self.all_nodes_phases_dict = {}
        for cur_obj_str in self.all_nodes_list:
            # ==Names
            cur_nd_obj_name_list = self.extract_attr("name", cur_obj_str)
            assert (
                    len(cur_nd_obj_name_list) == 1
            ), "Redundancy or missing on the name attribute!"
            self.all_nodes_names_list.append(cur_nd_obj_name_list[0])

            # ==Phases
            cur_nd_obj_phases_list = self.extract_attr("phases", cur_obj_str)
            # print(cur_nd_obj_phase_list)
            assert (
                    len(cur_nd_obj_phases_list) == 1
            ), "Redundancy or missing on the phase attribute!"
            self.all_nodes_phases_dict[
                cur_nd_obj_name_list[0]
            ] = cur_nd_obj_phases_list[0]

    def parse_node_3ph(self, lines_str):
        """Parse and Package All Node Objects
        """
        self.all_nodes_list = self.extract_obj(self.re_glm_syn_obj_node, lines_str)

        self.all_nodes_names_list = []
        self.all_nodes_phases_dict = {}

        self.all_nodes_names_3ph_list = []
        self.all_nodes_phases_3ph_dict = {}

        for cur_obj_str in self.all_nodes_list:
            # ==Names
            cur_nd_obj_name_list = self.extract_attr("name", cur_obj_str)
            assert (
                    len(cur_nd_obj_name_list) == 1
            ), "Redundancy or missing on the name attribute!"
            self.all_nodes_names_list.append(cur_nd_obj_name_list[0])

            # ==Phases
            cur_nd_obj_phases_list = self.extract_attr("phases", cur_obj_str)
            # print(cur_nd_obj_phase_list)
            assert (
                    len(cur_nd_obj_phases_list) == 1
            ), "Redundancy or missing on the phase attribute!"
            self.all_nodes_phases_dict[
                cur_nd_obj_name_list[0]
            ] = cur_nd_obj_phases_list[0]

            # == 3-Ph Nodes
            if cur_nd_obj_phases_list[0] == 'ABCN':
                self.all_nodes_names_3ph_list.append(cur_nd_obj_name_list[0])
                self.all_nodes_phases_3ph_dict[cur_nd_obj_name_list[0]] = cur_nd_obj_phases_list[0]

    def parse_load(self, lines_str):
        """Parse and Package All Load Objects
        """
        self.clean_load_buffer()
        # self.all_loads_list = re.findall(r'object\s*load.*?{(.*?)}',lines_str,flags=re.DOTALL)
        self.all_loads_list = re.findall(
            self.re_glm_syn_obj_load, lines_str, flags=re.DOTALL
        )

        self.all_loads_names_list = []
        self.all_loads_phases_dict = {}
        self.all_loads_p_sum = 0
        self.all_loads_q_sum = 0
        for cur_obj_str in self.all_loads_list:
            # print(cur_obj_str)

            # ==Names
            cur_ld_obj_name_list = self.extract_attr("name", cur_obj_str)
            assert (
                    len(cur_ld_obj_name_list) == 1
            ), "Redundancy or missing on the name attribute!"
            self.all_loads_names_list.append(cur_ld_obj_name_list[0])

            # ==Phases
            cur_ld_obj_phases_list = self.extract_attr("phases", cur_obj_str)
            assert (
                    len(cur_ld_obj_phases_list) == 1
            ), "Redundancy or missing on the phase attribute!"
            self.all_loads_phases_dict[
                cur_ld_obj_name_list[0]
            ] = cur_ld_obj_phases_list[0]

            # ==P & Q
            cur_ld_obj_sabc = [""] * 3
            # cur_ld_obj_sabc[0] = re.findall(r'.*constant_power_A\s*(.*?);',cur_obj_str,flags=re.DOTALL)
            cur_ld_obj_sabc[0] = self.extract_attr("constant_power_AN", cur_obj_str)
            cur_ld_obj_sabc[1] = re.findall(
                r".*constant_power_BN\s*(.*?);", cur_obj_str, flags=re.DOTALL
            )
            cur_ld_obj_sabc[2] = re.findall(
                r".*constant_power_CN\s*(.*?);", cur_obj_str, flags=re.DOTALL
            )
            # print(cur_ld_obj_sabc)
            cur_ld_obj_pabc = [0] * 3
            cur_ld_obj_qabc = [0] * 3
            for cur_ite in range(len(cur_ld_obj_sabc)):
                if cur_ld_obj_sabc[cur_ite]:
                    cur_ld_obj_pabc[cur_ite] = complex(cur_ld_obj_sabc[cur_ite][0]).real
                    cur_ld_obj_qabc[cur_ite] = complex(cur_ld_obj_sabc[cur_ite][0]).imag

            cur_obj_p_sum = 0
            for cur_ph_p in cur_ld_obj_pabc:
                cur_obj_p_sum += cur_ph_p

            cur_obj_q_sum = 0
            for cur_ph_q in cur_ld_obj_qabc:
                cur_obj_q_sum += cur_ph_q

            self.all_loads_p_list.append(cur_obj_p_sum)
            self.all_loads_p_sum += cur_obj_p_sum

            self.all_loads_q_list.append(cur_obj_q_sum)
            self.all_loads_q_sum += cur_obj_q_sum

    def parse_triload(self, lines_str):
        """Parse and Package All Load Objects
        """
        self.clean_load_buffer()
        self.all_loads_list = re.findall(
            r"object\s*triplex_load.*?{(.*?)}", lines_str, flags=re.DOTALL
        )

        for cur_obj_str in self.all_loads_list:
            cur_obj_s_list = re.findall(
                r".*constant_power_12\s*(.*?);", cur_obj_str, flags=re.DOTALL
            )

            cur_obj_p_sum = 0
            for cur_ph_s_str in cur_obj_s_list:
                cur_obj_p_sum += complex(cur_ph_s_str).real

            self.all_loads_p_list.append(cur_obj_p_sum)

    def parse_triplex_node(self, lines_str):
        """
        Note that this function takes the deprecated properties (e.g., power_1 & power_2) of the triplex_node object.
        The formal way is to define the load by using the triplex_load object, e.g., with constant power on phase 1 (120V).
        """
        self.clean_triplex_node_buffer()
        self.all_triplex_nodes_list = re.findall(
            r"object\s*triplex_node.*?{(.*?)}", lines_str, flags=re.DOTALL
        )

        # p1_w_sum = p2_w_sum = 0
        # q1_var_sum = q2_var_sum = 0
        for cur_obj_str in self.all_triplex_nodes_list:
            cur_obj_s1_list = re.findall(r".*power_1\s*(.*?);", cur_obj_str, flags=re.DOTALL)
            cur_obj_s2_list = re.findall(r".*power_2\s*(.*?);", cur_obj_str, flags=re.DOTALL)

            if cur_obj_s1_list:  # A triplex_node may not have loading defined
                if len(cur_obj_s1_list) == 1:
                    self.all_triplex_nodes_p1_list.append(complex(cur_obj_s1_list[0]).real)
                    self.all_triplex_nodes_q1_list.append(complex(cur_obj_s1_list[0]).imag)
                else:
                    raise ('Multiple power_1 values defined!')

            if cur_obj_s2_list:  # A triplex_node may not have loading defined
                if len(cur_obj_s2_list) == 1:
                    self.all_triplex_nodes_p2_list.append(complex(cur_obj_s2_list[0]).real)
                    self.all_triplex_nodes_q2_list.append(complex(cur_obj_s2_list[0]).imag)
                else:
                    raise ('Multiple power_2 values defined!')

    def del_cmts(self, ori_str):
        return re.sub(self.re_glm_syn_comm, "", ori_str)

    def del_mty_lns(self, ori_str):
        return re.sub(self.re_glm_syn_mty_lns, "", ori_str, flags=re.MULTILINE)

    def del_glm_objs(self, ori_str, re_glm_syn_obj):
        return re.sub(re_glm_syn_obj, "", ori_str, flags=re.DOTALL)

    def import_file(self, filename):
        o_file = open(filename, "r")
        str_file = o_file.read()
        o_file.close()

        str_file_woc = self.del_cmts(str_file)
        self.str_file_woc_copy = str_file_woc
        return str_file_woc

    def disp_triplex_node_info(self):
        total_p_w = sum(self.all_triplex_nodes_p1_list) + sum(self.all_triplex_nodes_p2_list)
        total_q_var = sum(self.all_triplex_nodes_q1_list) + sum(self.all_triplex_nodes_q2_list)

        print(f"Total Load on Triplex Nodes: {total_p_w / 1e3} (kW), {total_q_var / 1e3} (kVAR)")

    def disp_load_info(self):
        print(
            f"Total Load: {self.all_loads_p_sum / 1e3} (kW), {self.all_loads_q_sum / 1e3} (kVAR)"
        )

    def read_content_node(self, filename):
        str_file_woc = self.import_file(filename)
        self.parse_node(str_file_woc)

    def read_content_node_3ph(self, filename):
        str_file_woc = self.import_file(filename)
        self.parse_node_3ph(str_file_woc)

    def read_content_load(self, filename):
        """This func is added as an extra layer for flexible extension"""
        str_file_woc = self.import_file(filename)
        self.parse_load(str_file_woc)
        self.disp_load_info()

    def read_content_triload(self, filename):
        str_file_woc = self.import_file(filename)
        self.parse_triload(str_file_woc)

    def read_content_triplex_node(self, filename):
        str_file_woc = self.import_file(filename)
        self.parse_triplex_node(str_file_woc)
        self.disp_triplex_node_info()

    def read_inv_names(self, filename):
        str_file_woc = self.import_file(filename)
        self.parse_inv(str_file_woc)
        return self.all_invs_names_list

    def get_inv_glm_str(self, inv_id=1,
                        par_name_str='""',
                        inv_p_kw=5.0,
                        rated_kw=10.0,
                        inv_name_pref_str="INV",
                        inv_type_str="GFLW",
                        inv_ctrl_mode_str="GFL_CURRENT_SOURCE"):
        inv_temp_f_str = f'' \
                         f'// ////////////////////////////////// Inverter #{inv_id} M1/////////////////////////////////////////\n' \
                         f'object inverter_dyn {{\n' \
                         f'\tname {inv_name_pref_str}_{inv_type_str}_{inv_id};\n' \
                         f'\tparent {par_name_str};\n' \
                         f'\tflags DELTAMODE;\n' \
                         f'\tcontrol_mode {inv_ctrl_mode_str};\n' \
                         f'\t//control_mode GRID_FORMING;\n' \
                         f'\t//control_mode GFL_CURRENT_SOURCE;\n' \
                         f'\t//control_mode GRID_FOLLOWING;\n' \
                         f'\t//GFL_CURRENT_SOURCE_mode Balanced_Power;\n' \
                         f'\tgrid_following_mode POSITIVE_SEQUENCE;\n' \
                         f'\trated_power {rated_kw} kW;	//\n' \
                         f'\n' \
                         f'\tPref {inv_p_kw} kW; //1270000; //300000;\n' \
                         f'\tQref 0.0;\n' \
                         f'\n' \
                         f'\tRfilter 0.01;\n' \
                         f'\tXfilter 0.1;\n' \
                         f'\n' \
                         f'\t// Grid-Following Parameters //\n' \
                         f'\tfrequency_watt false;\n' \
                         f'\tcheckRampRate_real false;\n' \
                         f'\tvolt_var false;\n' \
                         f'\tcheckRampRate_reactive false;\n' \
                         f'\trampUpRate_real 1.67;  // pu/s\n' \
                         f'\trampDownRate_real 1.67; // pu/s\n' \
                         f'\trampUpRate_reactive 1.67; // pu/s\n' \
                         f'\trampDownRate_reactive 1.67; // pu/s\n' \
                         f'\tPref_max  1; // per unit\n' \
                         f'\tkpPLL  200;  // PLL gains\n' \
                         f'\tkiPLL  1000;\n' \
                         f'\tkpc  0.05;  // Current loop gains\n' \
                         f'\tkic 5;\n' \
                         f'\tF_current 0.5;\n' \
                         f'\t//Tpf 0.25;   // power response\n' \
                         f'\t//Tff 0.01;  // frequency measurement filter\n' \
                         f'\tRp 0.05;  // frequency-watt droop  1%\n' \
                         f'\tRq 0.05; // volt-var droop 5%\n' \
                         f'\t//Tqf 0.2;\n' \
                         f'\t//Tvf 0.05;\n' \
                         f'\t// End of Grid-Following Parameters //\n' \
                         f'\n' \
                         f'\t// Grid-Forming Parameters //\n' \
                         f'\tmp 3.77;  // 3.77 represents 1% droop.\n' \
                         f'\tkppmax 3;\n' \
                         f'\tkipmax 60;\n' \
                         f'\tPmax 1;\n' \
                         f'\tPmin -1;\n' \
                         f'\tmq 0.05; // 0.05 represents 5% droop\n' \
                         f'\t// End of Grid-Forming Parameters //\n' \
                         f'}};\n'
        return inv_temp_f_str

    def add_inverter_dyn(self, tar_glm_fpn, node_name_list,
                         num_inv=500, pct_gflw=1.0,
                         inv_gflw_p_kw=5.0, inv_gfrm_p_kw=5.0, rated_kw=10.0,
                         inv_name_pref_str="INV", inv_type_gflw_str="GFLW", inv_type_gfrm_str="GFRM",
                         inv_gflw_ctrl_mode_str="GFL_CURRENT_SOURCE", inv_gfrm_ctrl_mode_str="GRID_FORMING"):
        glm_str = ""

        # --prep
        num_gflw = round(num_inv * pct_gflw)
        num_gfrm = abs(num_inv - num_gflw)  # @TODO: simple sanity check
        num_inv = num_gflw + num_gfrm

        glm_info_str = f"//==Tolta number of inverters: {num_inv} " \
                       f"(Grid-Following: {num_gflw} [{num_gflw / num_inv * 100}%]) " \
                       f"(Grid-Forming: {num_gfrm} [{num_gfrm / num_inv * 100}%])==\n\n"
        glm_str += glm_info_str

        # --select nodes
        node_samp_list = random.sample(node_name_list, num_inv)
        node_gflw_samp_list = node_samp_list[0:num_gflw]
        node_gfrm_samp_list = node_samp_list[num_gflw:num_inv]

        # --add inv
        # ~~gflw
        glm_inv_gflw_str = ''

        inv_gflw_id = 1
        for cur_node_name_str in node_gflw_samp_list:
            cur_inv_gflw_str = self.get_inv_glm_str(inv_id=inv_gflw_id,
                                                    par_name_str=cur_node_name_str,
                                                    inv_p_kw=inv_gflw_p_kw,
                                                    inv_name_pref_str=inv_name_pref_str,
                                                    rated_kw=rated_kw,
                                                    inv_type_str=inv_type_gflw_str,
                                                    inv_ctrl_mode_str=inv_gflw_ctrl_mode_str)
            glm_inv_gflw_str += cur_inv_gflw_str
            inv_gflw_id += 1

        # ~~gfrm
        glm_inv_gfrm_str = ''

        inv_gfrm_id = 1
        for cur_node_name_str in node_gfrm_samp_list:
            cur_inv_gfrm_str = self.get_inv_glm_str(inv_id=inv_gfrm_id,
                                                    par_name_str=cur_node_name_str,
                                                    inv_p_kw=inv_gfrm_p_kw,
                                                    inv_name_pref_str=inv_name_pref_str,
                                                    rated_kw=rated_kw,
                                                    inv_type_str=inv_type_gfrm_str,
                                                    inv_ctrl_mode_str=inv_gfrm_ctrl_mode_str)
            glm_inv_gfrm_str += cur_inv_gfrm_str
            inv_gfrm_id += 1

        # ~~merge
        glm_str += glm_inv_gflw_str + glm_inv_gfrm_str

        # --export glm
        self.export_glm(tar_glm_fpn, glm_str)

    def set_inverters(self, csv_qout_fpn, glm_inv_src_fp, glm_inv_src_fn, glm_inv_dst_fp, glm_inv_dst_fn):
        # ==Step 01: Read csv file
        with open(csv_qout_fpn) as csvfile:
            qout_csv_reader = csv.reader(csvfile)
            next(qout_csv_reader, None)  # skip the headers

            inv_qout_dict = {}
            for cur_row in qout_csv_reader:
                # print(cur_row)
                cur_inv_name = cur_row[0]
                cur_inv_qout_var = cur_row[1]

                inv_qout_dict[cur_inv_name] = cur_inv_qout_var

        # ==Step 02: Update the glm file with inverters
        glm_inv_src_fpn = os.path.join(glm_inv_src_fp, glm_inv_src_fn)
        glm_inv_dst_fpn = os.path.join(glm_inv_dst_fp, glm_inv_dst_fn)

        # --read contents of the inv glm file
        glm_inv_src_str = self.import_file(glm_inv_src_fpn)
        glm_inv_dst_str = glm_inv_src_str

        # --search the inverters
        for cur_inv_name, cur_inv_qout in inv_qout_dict.items():
            cur_inv_glm_lines_list, cur_inv_re_tpl = self.find_obj_via_attr(
                "inverter", "name", cur_inv_name, glm_inv_src_str)

            if len(cur_inv_glm_lines_list) == 1:
                cur_inv_glm_lines_str = cur_inv_glm_lines_list[0]
            else:
                raise ValueError("The source glm is problematic")

            # ~~update Q_Out
            cur_q_var_str = f"{cur_inv_qout}"
            cur_inv_glm_lines_mod_str = self.modify_attr(
                "Q_Out", cur_q_var_str, cur_inv_glm_lines_str)

            # ~~replace the obj portion in the source string
            glm_inv_dst_str = self.replace_obj(
                cur_inv_re_tpl, glm_inv_dst_str, cur_inv_glm_lines_mod_str)

        # --export glm
        self.export_glm(glm_inv_dst_fpn, glm_inv_dst_str)

        # --run GLD, and save csv files
        # self.run_gld()

        # # ~~put under one folder
        # cur_results_flr_pfn = self.stor_csv_path
        # self.move_csv_files(cur_results_flr_pfn)

    def add_ufls_gfas(
            self,
            output_glm_path_fn,
            ufls_pct,
            ufls_th,
            ufls_dly,
            gfa_rc_time,
            gfa_extra_str="",
            flag_triload=False,
            flag_des=True,
    ):
        """ flag_des is reserved for extension
        """
        sorted_ind_list = sorted(
            range(len(self.all_loads_p_list)),
            key=lambda k: self.all_loads_p_list[k],
            reverse=flag_des,
        )

        total_loads_p = sum(self.all_loads_p_list)
        ufls_p = [x / 100 * total_loads_p for x in ufls_pct]
        ufls_p_asg = [0] * len(ufls_p)
        ufls_gfa_num = [0] * len(ufls_p)
        all_loads_ufls_tag = [-1] * len(self.all_loads_p_list)

        for cur_ite in range(len(ufls_p)):
            for cur_ite_ld in range(len(sorted_ind_list)):
                cur_sorted_ind = sorted_ind_list[cur_ite_ld]
                if all_loads_ufls_tag[cur_sorted_ind] >= 0:
                    continue
                cur_load_p = self.all_loads_p_list[cur_sorted_ind]
                if cur_load_p == 0:
                    continue
                if (ufls_p_asg[cur_ite] + cur_load_p) <= ufls_p[cur_ite]:
                    ufls_p_asg[cur_ite] += cur_load_p
                    ufls_gfa_num[cur_ite] += 1
                    all_loads_ufls_tag[cur_sorted_ind] = cur_ite

        # ==Display & Export
        if flag_triload:
            print("\n[Triplex Loads:]")
        else:
            print("\n[Loads:]")
        print("Total Load in kW: {}".format(total_loads_p / 1000))
        print("Assigned UFLS in kW: {}".format([x / 1000 for x in ufls_p_asg]))
        print(
            "Assigned UFLS in %: {}".format(
                [x / total_loads_p * 100 for x in ufls_p_asg]
            )
        )
        print("Number of GFA devices: {}".format(ufls_gfa_num))

        self.export_glm_with_gfas(
            output_glm_path_fn,
            all_loads_ufls_tag,
            ufls_th,
            ufls_dly,
            gfa_rc_time,
            gfa_extra_str,
            flag_triload,
        )

    def prepare_export_file(self, file_pn):
        if os.path.exists(file_pn):
            os.remove(file_pn)
            print("The old '{}' file is deleted!".format(file_pn))

        hf_output = open(file_pn, "w")
        print("The new '{}' file is created!".format(file_pn))
        return hf_output

    def export_glm_with_gfas(
            self,
            output_glm_path_fn,
            all_loads_ufls_tag,
            ufls_th,
            ufls_dly,
            gfa_rc_time,
            gfa_extra_str,
            flag_triload,
    ):
        hf_output = self.prepare_export_file(output_glm_path_fn)
        for ld_str, gp in zip(self.all_loads_list, all_loads_ufls_tag):
            # print(ld_str)
            if gp >= 0:
                cur_gfa_str = self.sobj_gfa_tpl_str.format(
                    ufls_th[gp], gfa_rc_time, ufls_dly[gp], gfa_extra_str
                )
            else:
                cur_gfa_str = ""

            if flag_triload:
                hf_output.write(self.obj_triload_tpl_str.format(ld_str, cur_gfa_str))
            else:
                hf_output.write(self.obj_load_tpl_str.format(ld_str, cur_gfa_str))
        hf_output.close()

    def export_glm(self, output_glm_path_fn, str_to_glm):
        hf_output = self.prepare_export_file(output_glm_path_fn)
        hf_output.write(str_to_glm)
        hf_output.close()

    def separate_load_objs(
            self, glm_file_path_fn, output_main_glm_path_fn, output_load_glm_path_fn
    ):
        self.read_content_load(glm_file_path_fn)

        output_str = ""
        for cur_str in self.all_loads_list:
            output_str += f"object load {{{cur_str}}}\n"
        self.export_glm(output_load_glm_path_fn, output_str)

        str_main_file_noloads = self.del_glm_objs(
            self.str_file_woc_copy, self.re_glm_syn_obj_load
        )
        str_main_file_noloads = self.del_mty_lns(str_main_file_noloads)

        self.export_glm(output_main_glm_path_fn, str_main_file_noloads)

    def adjust_load_amount(self, load_glm_path_fn, adj_load_glm_path_fn, tgt_p, tgt_pf):
        """tgt_p is in kW"""
        self.read_content_load(load_glm_path_fn)
        p_ratio = tgt_p / (self.all_loads_p_sum / 1e3)

        print(f"Ratio: {p_ratio}")
        print(f"Total Load after Adjustment: {p_ratio * self.all_loads_p_sum / 1e3} (kW)")
        print(f"Power Factor: {tgt_pf}\n")

        self.all_adj_loads_list = []
        out_adj_load_str = ""
        for cur_obj_str in self.all_loads_list:
            cur_new_obj_str = self.update_pq(cur_obj_str, p_ratio, tgt_pf)
            self.all_adj_loads_list.append(cur_new_obj_str)

            out_adj_load_str += self.obj_load_tpl_str.format(cur_new_obj_str, "")

        self.export_glm(adj_load_glm_path_fn, out_adj_load_str)

    def update_pq_type(self, cur_triplex_node_str, ld_mult, ld_type):  # TODO: to be deleted
        # == Step 01: Check the load conversion type
        if ld_type == 'p_to_z':
            ori_power_atts_list = ['power_1', 'power_2']
            new_power_atts_list = ['impedance_1', 'impedance_2']
        else:
            raise (f"The load type defined in 'ld_type' is not supported yet!")

        # == Step 02: Update/Adjust
        cur_new_triplex_node_str = cur_triplex_node_str

        for cur_ori_power_att_str, cur_new_power_att_str in zip(ori_power_atts_list, new_power_atts_list):
            cur_att_re_str = fr".*{cur_ori_power_att_str}\s*(.*?);"
            cur_ph_s_lt = re.findall(cur_att_re_str, cur_new_triplex_node_str, flags=re.DOTALL)
            if cur_ph_s_lt:
                cur_ph_p_w = complex(cur_ph_s_lt[0]).real
                cur_ph_q_var = complex(cur_ph_s_lt[0]).imag

                new_ph_p_w = ld_mult * cur_ph_p_w
                new_ph_q_var = ld_mult * cur_ph_q_var

                cur_nominal_volt_list = self.extract_attr('nominal_voltage', cur_triplex_node_str)
                if len(cur_nominal_volt_list) != 1:
                    raise ("Nomianl voltage is not defined well.")
                else:
                    cur_nominal_volt_v = float(cur_nominal_volt_list[0])

                new_s_abs_w = math.sqrt(new_ph_p_w ** 2 + new_ph_q_var ** 2)
                new_z_abs_ohm = cur_nominal_volt_v ** 2 / new_s_abs_w
                new_pf_theta_rad = math.asin(new_ph_q_var / new_s_abs_w)
                new_ph_r_ohm = new_z_abs_ohm * math.cos(new_pf_theta_rad)
                new_ph_x_ohm = new_z_abs_ohm * math.sin(new_pf_theta_rad)

                new_ph_s_str = f"\t{cur_new_power_att_str} {new_ph_r_ohm}+{new_ph_x_ohm}j;"

                cur_new_triplex_node_str = re.sub(
                    cur_att_re_str, new_ph_s_str, cur_new_triplex_node_str, flags=re.MULTILINE
                )

        return cur_new_triplex_node_str

    def update_pq(self, cur_obj_str, p_ratio, tgt_pf):
        # print(cur_obj_str)
        power_atts_list = ["constant_power_A", "constant_power_B", "constant_power_C"]
        cur_new_obj_str = cur_obj_str
        for cur_att_str in power_atts_list:
            cur_att_re_str = fr".*{cur_att_str}\s*(.*?);"
            cur_ph_s_lt = re.findall(cur_att_re_str, cur_new_obj_str, flags=re.DOTALL)
            if cur_ph_s_lt:
                cur_ph_p = complex(cur_ph_s_lt[0]).real
                cur_ph_q = complex(cur_ph_s_lt[0]).imag
                new_ph_p = p_ratio * cur_ph_p
                new_ph_q = self.get_q(new_ph_p, tgt_pf)
                new_ph_s_str = f"\t{cur_att_str} {new_ph_p}+{new_ph_q}j;"
                # print(new_ph_s_str + '~~~~~~~')
                cur_new_obj_str = re.sub(
                    cur_att_re_str, new_ph_s_str, cur_new_obj_str, flags=re.MULTILINE
                )
                # print(cur_new_obj_str)
                # print('====\n')
        return cur_new_obj_str

    def get_q(self, p, pf):
        return ((1 - pf * pf) ** 0.5) / pf * p

    def read_zone_info(self, obj_zone_csv_path_fn):
        # ==Test & Demo
        with open(obj_zone_csv_path_fn) as csvfile:
            obj_zone_csv_reader = csv.reader(csvfile)
            next(obj_zone_csv_reader, None)  # skip the headers

            obj_zone_dict = {}
            obj_zone_missing_list = []
            for cur_row in obj_zone_csv_reader:
                cur_obj_name = cur_row[0]
                cur_obj_zone_str = cur_row[1]
                cur_obj_zone_int = [
                    int(x) for x in cur_obj_zone_str.split() if x.isdigit()
                ]
                if not cur_obj_zone_int:
                    obj_zone_dict[cur_obj_name] = "NONE"
                    obj_zone_missing_list.append(cur_obj_name)
                else:
                    assert len(cur_obj_zone_int) == 1, "Redundancy of Segment Info"
                    obj_zone_dict[cur_obj_name] = cur_obj_zone_int[0]
            return obj_zone_dict, obj_zone_missing_list

    def update_zip_type(self, cur_triplex_node_str, ld_mult, ld_type,
                        p_pf=0.9, i_pf=0.9, z_pf=0.9, p_pct=1.0, i_pct=0, z_pct=0,
                        prefix_triplex_load_obj_str='tpld_'):
        # == Step 01: Check the load conversion type
        if ld_type == 'p_to_z':
            ori_power_atts_list = ['power_1', 'power_2']
            new_power_atts_list = ['impedance_1', 'impedance_2']
        elif ld_type == 'p_to_zip':
            ori_power_atts_list = ['power_1', 'power_2']  # @TODO: power_12 is omitted in this version
            new_power_atts_list = ['_1', '_2']
        else:
            raise (f"The load type defined in 'ld_type' is not supported yet!")

        # == Step 02: Update/Adjust
        cur_new_triplex_node_str = cur_triplex_node_str
        cur_new_triplex_load_str = ""

        for cur_ori_power_att_str, cur_new_power_att_str in zip(ori_power_atts_list, new_power_atts_list):
            cur_att_re_str = fr".*{cur_ori_power_att_str}\s*(.*?);"
            cur_ph_s_lt = re.findall(cur_att_re_str, cur_new_triplex_node_str, flags=re.DOTALL)
            if cur_ph_s_lt:
                cur_ph_p_w = complex(cur_ph_s_lt[0]).real
                cur_ph_q_var = complex(cur_ph_s_lt[0]).imag

                new_ph_p_w = ld_mult * cur_ph_p_w
                new_ph_q_var = ld_mult * cur_ph_q_var

                cur_nominal_volt_list = self.extract_attr('nominal_voltage', cur_triplex_node_str)
                if len(cur_nominal_volt_list) != 1:
                    raise ("Nomianl voltage is not defined well.")
                else:
                    cur_nominal_volt_v = float(cur_nominal_volt_list[0])

                new_s_abs_w = math.sqrt(new_ph_p_w ** 2 + new_ph_q_var ** 2)

                if ld_type == 'p_to_z':
                    new_z_abs_ohm = cur_nominal_volt_v ** 2 / new_s_abs_w
                    new_pf_theta_rad = math.asin(new_ph_q_var / new_s_abs_w)
                    new_ph_r_ohm = new_z_abs_ohm * math.cos(new_pf_theta_rad)
                    new_ph_x_ohm = new_z_abs_ohm * math.sin(new_pf_theta_rad)

                    new_ph_s_str = f"\t{cur_new_power_att_str} {new_ph_r_ohm}+{new_ph_x_ohm}j;"
                elif ld_type == 'p_to_zip':
                    new_ph_s_str = ""

                    # -- name, phases, parent
                    if not cur_new_triplex_load_str:
                        cur_name_list = self.extract_attr('name', cur_triplex_node_str)
                        if len(cur_name_list) != 1:
                            raise ("Name is not defined well.")
                        else:
                            cur_name_str = cur_name_list[0]
                            cur_name_no_quote_str = cur_name_str.strip('\"')

                        cur_phases_list = self.extract_attr('phases', cur_triplex_node_str)
                        if len(cur_phases_list) != 1:
                            raise ("Phases attribute is not defined well.")
                        else:
                            cur_phases_str = cur_phases_list[0]

                        cur_new_triplex_load_str += f'\tname "{prefix_triplex_load_obj_str}{cur_name_no_quote_str}";\n' \
                                                    f"\tparent {cur_name_str};\n" \
                                                    f"\tnominal_voltage {cur_nominal_volt_v};\n" \
                                                    f"\tphases {cur_phases_str};\n" \
                                                    f"\n"

                    # -- zip properties
                    cur_new_triplex_load_str += f"\tbase_power{cur_new_power_att_str} {new_s_abs_w};\n" \
                                                f"\n" \
                                                f"\tpower_pf{cur_new_power_att_str} {p_pf};\n" \
                                                f"\tcurrent_pf{cur_new_power_att_str} {i_pf};\n" \
                                                f"\timpedance_pf{cur_new_power_att_str} {z_pf};\n" \
                                                f"\n" \
                                                f"\tpower_fraction{cur_new_power_att_str} {p_pct};\n" \
                                                f"\tcurrent_fraction{cur_new_power_att_str} {i_pct};\n" \
                                                f"\timpedance_fraction{cur_new_power_att_str} {z_pct};\n" \
                                                f"\n"

                cur_new_triplex_node_str = re.sub(cur_att_re_str, new_ph_s_str, cur_new_triplex_node_str,
                                                  flags=re.MULTILINE)

        return cur_new_triplex_node_str, cur_new_triplex_load_str

    def adjust_triplex_nodes(self, triplex_node_glm_path_fn, adj_triplex_node_glm_path_fn, ld_mult, ld_type):
        # == Step 01: Read the contents of triplex node from a glm file
        self.read_content_triplex_node(triplex_node_glm_path_fn)

        # == Step 02: Display loading info
        info_ratio_str = f"Ratio (of new to original loading): {ld_mult}"
        print(info_ratio_str)

        total_p_w = sum(self.all_triplex_nodes_p1_list) + sum(self.all_triplex_nodes_p2_list)
        total_q_var = sum(self.all_triplex_nodes_q1_list) + sum(self.all_triplex_nodes_q2_list)
        info_total_adj_load_str = f"Total Load (adjusted): {ld_mult * total_p_w / 1e3} (kW), {ld_mult * total_q_var / 1e3} (kVAR)"
        info_total_ori_load_str = f"Total Load (original): {total_p_w / 1e3} (kW), {total_q_var / 1e3} (kVAR)"
        print(info_total_adj_load_str)
        print(info_total_ori_load_str)

        # == Step 03: Adjust the load type and amount
        self.all_adj_triplex_nodes_list = []
        output_adj_triplex_nodes_str = f"// Triplex_node objects with adjusted loads\n" \
                                       f"// {info_ratio_str}\n" \
                                       f"// {info_total_adj_load_str}\n" \
                                       f"// {info_total_ori_load_str}\n\n"
        for cur_obj_str in self.all_triplex_nodes_list:
            # cur_new_triplex_node_obj_str = self.update_pq_type(cur_obj_str, ld_mult, ld_type)
            cur_new_triplex_node_obj_str, _ = self.update_zip_type(cur_obj_str, ld_mult, ld_type)
            self.all_adj_triplex_nodes_list.append(cur_new_triplex_node_obj_str)

            output_adj_triplex_nodes_str += self.obj_triplex_node_tpl_str.format(cur_new_triplex_node_obj_str, "")

        # == Step 04: Export & save to a file
        self.export_glm(adj_triplex_node_glm_path_fn, output_adj_triplex_nodes_str)

    def add_parallel_cables(self, scl_pcs_names_list, tar_glm_fpn, pcs_glm_fpn):
        str_file_woc = self.import_file(tar_glm_fpn)
        pcs_glm_str = ''
        counter_pcs = 0
        for cur_pc_name in scl_pcs_names_list:
            cur_pc_extr_obj_list, _ = self.find_obj_via_attr('overhead_line', 'name',
                                                             '"line_' + cur_pc_name.lower() + '"',
                                                             str_file_woc)
            if cur_pc_extr_obj_list:
                if len(cur_pc_extr_obj_list) != 1:
                    raise Exception('Duplicate Names!')
                cur_pc_obj_str = cur_pc_extr_obj_list[0]

                # == Get Length
                cur_pc_length_str = self.extract_attr('length', cur_pc_obj_str)
                if len(cur_pc_length_str) != 1:
                    raise Exception('Length Issue!')
                cur_pc_length = float(cur_pc_length_str[0])

                # == Add Comment
                counter_pcs += 1
                pcs_glm_str += f'//-- PC_{counter_pcs}: {cur_pc_name}\n'

                cur_pc_node_name_str = '"node_' + cur_pc_name.lower() + '_PCN"'

                # == Add PC_01
                cur_pc_01_obj_str = self.modify_attr('length', str(cur_pc_length / 2), cur_pc_obj_str)
                cur_pc_01_obj_str = self.modify_attr('name', '"line_' + cur_pc_name.lower() + '_PC_01"',
                                                     cur_pc_01_obj_str)
                cur_pc_01_obj_str = self.modify_attr('to', cur_pc_node_name_str, cur_pc_01_obj_str)

                pcs_glm_str += cur_pc_01_obj_str + '\n'

                # == Add PC_02
                cur_pc_02_obj_str = self.modify_attr('length', str(cur_pc_length / 2), cur_pc_obj_str)
                cur_pc_02_obj_str = self.modify_attr('name', '"line_' + cur_pc_name.lower() + '_PC_02"',
                                                     cur_pc_02_obj_str)
                cur_pc_02_obj_str = self.modify_attr('from', cur_pc_node_name_str,
                                                     cur_pc_02_obj_str)

                pcs_glm_str += cur_pc_02_obj_str + '\n'

                # == Add PC_Mid_Node
                cur_pc_obj_from_node_name_list = self.extract_attr('from', cur_pc_obj_str)
                if len(cur_pc_obj_from_node_name_list) != 1:
                    raise Exception('Length Issue!')
                cur_pc_from_node_obj_list, _ = self.find_obj_via_attr('node', 'name', cur_pc_obj_from_node_name_list[0],
                                                                      str_file_woc)

                if len(cur_pc_from_node_obj_list) != 1:
                    raise Exception('Node Issue!')

                cur_pc_from_node_obj_str = cur_pc_from_node_obj_list[0]
                cur_pc_node_obj_str = self.modify_attr('name', cur_pc_node_name_str, cur_pc_from_node_obj_str)

                pcs_glm_str += cur_pc_node_obj_str + '\n'

        pcs_glm_str = f'//== {pcs_glm_fpn} PCs (Number: {counter_pcs})\n' + pcs_glm_str
        self.export_glm(pcs_glm_fpn, pcs_glm_str)

    def add_triplex_loads(self, triplex_node_glm_path_fn, adj_triplex_node_glm_path_fn, ld_mult, ld_type,
                          p_pf=0.9, i_pf=0.9, z_pf=0.9, p_pct=1.0, i_pct=0, z_pct=0):
        # == Step 01: Read the contents of triplex node from a glm file
        self.read_content_triplex_node(triplex_node_glm_path_fn)

        # == Step 02: Display loading info
        info_ratio_str = f"Ratio (of new to original loading): {ld_mult}"
        print(info_ratio_str)

        total_p_w = sum(self.all_triplex_nodes_p1_list) + sum(self.all_triplex_nodes_p2_list)
        total_q_var = sum(self.all_triplex_nodes_q1_list) + sum(self.all_triplex_nodes_q2_list)
        info_total_adj_load_str = f"Total Load (adjusted): {ld_mult * total_p_w / 1e3} (kW), {ld_mult * total_q_var / 1e3} (kVAR)"
        info_total_ori_load_str = f"Total Load (original): {total_p_w / 1e3} (kW), {total_q_var / 1e3} (kVAR)"
        print(info_total_adj_load_str)
        print(info_total_ori_load_str)

        # == Step 03: Adjust the load type and amount
        self.all_adj_triplex_nodes_list = []
        output_adj_triplex_nodes_str = f"// Triplex_node objects with adjusted loads (modeled as triplex_load objects)\n" \
                                       f"// {info_ratio_str}\n" \
                                       f"// {info_total_adj_load_str}\n" \
                                       f"// {info_total_ori_load_str}\n\n"
        for cur_obj_str in self.all_triplex_nodes_list:
            cur_new_triplex_node_obj_str, \
            cur_new_triplex_load_obj_str = self.update_zip_type(cur_obj_str, ld_mult, ld_type,
                                                                p_pf, i_pf, z_pf, p_pct, i_pct, z_pct)

            self.all_adj_triplex_nodes_list.append(cur_new_triplex_node_obj_str)  # @TODO: not needed

            # output_adj_triplex_nodes_str += self.obj_triplex_node_tpl_str.format(cur_new_triplex_node_obj_str, "")
            output_adj_triplex_nodes_str += self.obj_tpl_str.format("triplex_node", cur_new_triplex_node_obj_str, "")
            if cur_new_triplex_load_obj_str:
                output_adj_triplex_nodes_str += \
                    self.obj_tpl_str.format("triplex_load", cur_new_triplex_load_obj_str, "")

        # == Step 04: Export & save to a file
        self.export_glm(adj_triplex_node_glm_path_fn, output_adj_triplex_nodes_str)


def test_add_ufls_gfas():
    # ==Parameters
    # Note that the filename cannot end with slash(es)
    glm_file_path_fn = "loads.glm"
    output_glm_path_fn = "loads_gfa.glm"

    ufls_pct = [10, 15, 10]  # Unit: %
    ufls_th = [59.0, 58.5, 58.0]  # Unit: Hz
    ufls_dly = [0.1, 0.05, 0.08]  # Unit: sec

    # Unit: sec #Set this to a large value to avoid reconnection.
    gfa_rc_time = 1000
    gfa_extra_str = "\n\
    GFA_freq_high_trip 70 Hz;\n\
    GFA_volt_low_trip 0.5;\n\
    GFA_volt_high_trip 1.5;\n\
    GFA_volt_disconnect_time 0.08 s;"

    # ==Test & Demo
    p = GlmParser()

    p.read_content_load(glm_file_path_fn)
    p.add_ufls_gfas(
        output_glm_path_fn, ufls_pct, ufls_th, ufls_dly, gfa_rc_time, gfa_extra_str
    )
    """
    p.read_content_triload('triplex_loads.glm')
    triload_flag = True
    p.add_ufls_gfas('triplex_loads_gfa.glm',ufls_pct,ufls_th,ufls_dly,gfa_rc_time,triload_flag)
    """


def test_separate_load_objs():
    # ==Parameters
    glm_file_path_fn = r"D:\UC3_S1_Tap12_[with MG][Clean][LessLoad]\Duke_4F_Aug30.glm"
    output_main_glm_path_fn = (
        r"D:\UC3_S1_Tap12_[with MG][Clean][LessLoad]\Duke_Main.glm"
    )
    output_load_glm_path_fn = (
        r"D:\UC3_S1_Tap12_[with MG][Clean][LessLoad]\duke_loads.glm"
    )

    # ==Test & Demo
    p = GlmParser()

    p.separate_load_objs(
        glm_file_path_fn, output_main_glm_path_fn, output_load_glm_path_fn
    )


def test_adjust_load_amount():
    # ==Parameters
    load_glm_path_fn = r"D:\UC3_S1_Tap12_[with MG][Clean][LessLoad]\duke_loads.glm"
    adj_load_glm_path_fn = (
        r"D:\UC3_S1_Tap12_[with MG][Clean][LessLoad]\duke_loads_adj.glm"
    )
    utk_p = [7978, 2825, 6842, 6530]  # Unit: kW
    utk_p_sum = sum(utk_p)  # Unit: kW
    utk_pf = 0.98

    # ==Test & Demo
    p = GlmParser()

    p.adjust_load_amount(load_glm_path_fn, adj_load_glm_path_fn, utk_p_sum, utk_pf)


def test_read_content_load():
    # ==Parameters
    # load_glm_path_fn = r"D:\UC3_S1_Tap12_[with MG][Clean][LessLoad]\duke_loads_adj.glm"
    load_glm_path_fn = r"C:\Users\xiej204\OneDrive - PNNL\Desktop\PNNL\Sid\2021-2-25_GLD Model_SC & SV\PGE_Feeder_SV.glm"

    # ==Test & Demo
    p = GlmParser()
    p.read_content_load(load_glm_path_fn)

    print(p.all_loads_names_list[0:5])


def test_read_content_node():
    # ==Parameters
    main_glm_path_fn = r"D:\UC3_S1_Tap12_[with MG][Clean][LessLoad]\Duke_Main.glm"

    # ==Test & Demo
    p = GlmParser()
    p.read_content_node(main_glm_path_fn)

    print(p.all_nodes_names_list[0:2])


def test_read_phase_info():
    pass


def test_read_zone_info():
    # ==Parameters
    node_zone_csv_path_fn = r"D:\Duke\Zone ID\Base Case - Nodes Report.csv"
    load_zone_csv_path_fn = r"D:\Duke\Zone ID\Base Case - Loads Report.csv"

    # ==Test & Demo
    p = GlmParser()

    # --nodes
    node_zone_dict, node_zone_missing_list = p.read_zone_info(node_zone_csv_path_fn)

    # print(node_zone_dict['256824001'])
    # print(node_zone_dict['260440608'])
    # print(node_zone_dict['256892070'])
    print(node_zone_missing_list)

    # --loads
    load_zone_dict, load_zone_missing_list = p.read_zone_info(load_zone_csv_path_fn)
    # print(load_zone_dict)
    print(load_zone_missing_list)

    # ==
    return p, node_zone_dict, load_zone_dict


def test_mapping_zone_info():
    p, node_zone_dict, load_zone_dict = test_read_zone_info()

    # ==Parameters
    main_glm_path_fn = r"D:\UC3_S1_Tap12_[with MG][Clean][LessLoad]\Duke_Main.glm"
    load_glm_path_fn = r"D:\UC3_S1_Tap12_[with MG][Clean][LessLoad]\duke_loads_adj.glm"

    zone_info_dicts_pickle_path_fn = (
        r"D:\UC3_S1_Tap12_[with MG][Clean][LessLoad]\zone_info"
    )

    # ==Test & Demo
    new_node_zone_dict = {}
    new_node_zone_missing_list = []
    new_load_zone_dict = {}
    new_load_zone_missing_list = []

    new_node_zone_phase_dict = {}
    new_load_zone_phase_dict = {}

    # --new zone nodes
    new_zone_node_3ph_dict = {}

    # --node mapping
    p.read_content_node(main_glm_path_fn)

    for cur_node_name_str in p.all_nodes_names_list:
        cur_node_name_feeder_list = re.findall("\d+", cur_node_name_str)
        # @TODO: assert & check
        cur_node_name_key = cur_node_name_feeder_list[0]
        if cur_node_name_key in node_zone_dict.keys():
            cur_zone_info = node_zone_dict[cur_node_name_key]
            new_node_zone_dict[cur_node_name_str] = cur_zone_info
            cur_phase_str = p.all_nodes_phases_dict[cur_node_name_str]
            new_node_zone_phase_dict[cur_node_name_str] = [cur_zone_info, cur_phase_str]

            # --new_zone_node_3ph_dict
            if (
                    ("A" in cur_phase_str)
                    and ("B" in cur_phase_str)
                    and ("C" in cur_phase_str)
            ):
                if cur_zone_info in new_zone_node_3ph_dict.keys():
                    new_zone_node_3ph_dict[cur_zone_info].append(
                        [cur_node_name_str, cur_phase_str]
                    )
                else:
                    new_zone_node_3ph_dict[cur_zone_info] = [
                        [cur_node_name_str, cur_phase_str]
                    ]
        else:
            new_node_zone_missing_list.append(cur_node_name_str)
            # print(cur_node_name_key)
            # print(cur_node_name_str)

    print(new_node_zone_missing_list)

    # print(p.all_nodes_phases_dict)
    # print(new_node_zone_dict)
    # print(new_node_zone_phase_dict)

    print(new_node_zone_phase_dict["n256851437_1207"])

    # --accounting
    seg_loading_p_dict = {}
    seg_loading_q_dict = {}

    # --load mapping
    p.read_content_load(load_glm_path_fn)

    for cur_load_name_str, cur_load_p, cur_load_q in zip(
            p.all_loads_names_list, p.all_loads_p_list, p.all_loads_q_list
    ):
        cur_load_name_feeder_list = re.findall("\d+", cur_load_name_str)
        cur_load_name_key = cur_load_name_feeder_list[0]
        if cur_load_name_key in load_zone_dict.keys():
            cur_zone_info = load_zone_dict[cur_load_name_key]
            new_load_zone_dict[cur_load_name_str] = cur_zone_info
            cur_phase_str = p.all_loads_phases_dict[cur_load_name_str]
            new_load_zone_phase_dict[cur_load_name_str] = [cur_zone_info, cur_phase_str]

            if cur_zone_info in seg_loading_p_dict.keys():
                seg_loading_p_dict[cur_zone_info] += cur_load_p
                seg_loading_q_dict[cur_zone_info] += cur_load_q
            else:
                seg_loading_p_dict[cur_zone_info] = cur_load_p
                seg_loading_q_dict[cur_zone_info] = cur_load_q
        else:
            new_load_zone_missing_list.append(cur_load_name_str)

    print("~~~~~~~~Segment Loading:")
    print(seg_loading_p_dict)

    print("~~~~~~~~Loads that have no segment assigned:")
    print(new_load_zone_missing_list)
    # print(new_load_zone_phase_dict)

    print(new_load_zone_phase_dict["39693222_1207"])
    print(new_load_zone_phase_dict["39695307_1207"])

    # ==Save
    hf_zone_info = open(zone_info_dicts_pickle_path_fn, "wb")
    # pickle.dump(new_node_zone_dict, hf_zone_info)
    # pickle.dump(new_load_zone_dict, hf_zone_info)
    pickle.dump(new_node_zone_phase_dict, hf_zone_info)
    pickle.dump(new_load_zone_phase_dict, hf_zone_info)
    hf_zone_info.close()

    print(len(new_node_zone_dict))
    print(len(new_load_zone_dict))

    return new_zone_node_3ph_dict, seg_loading_p_dict, seg_loading_q_dict


def test_load_zone_info():
    zone_info_dicts_pickle_path_fn = (
        r"D:\UC3_S1_Tap12_[with MG][Clean][LessLoad]\zone_info"
    )
    hf_zone_info = open(zone_info_dicts_pickle_path_fn, "rb")
    new_node_zone_dict = pickle.load(hf_zone_info)
    print(len(new_node_zone_dict))
    new_load_zone_dict = pickle.load(hf_zone_info)
    print(len(new_load_zone_dict))
    hf_zone_info.close()


def test_pick_node_from_segments():
    # ==Parameters
    num_selected_pv_nodes_per_zone = 5
    random_seed = 2

    random.seed(random_seed)

    # ==Test & Demo
    new_zone_node_3ph_dict = test_mapping_zone_info()

    # print(new_zone_node_3ph_dict)
    # print(len(new_zone_node_3ph_dict))

    for cur_zone_key, cur_zone_value in new_zone_node_3ph_dict.items():
        # print(len(cur_zone_value))
        # cur_selected_lists = random.choices(cur_zone_value, k=num_selected_pv_nodes_per_zone)
        cur_selected_lists = random.sample(
            cur_zone_value, num_selected_pv_nodes_per_zone
        )
        # print(cur_selected_lists)
        for cur_node in cur_selected_lists:
            cur_node_name_str = cur_node[0]
            cur_node_phase_str = cur_node[1]
            print(f"{cur_zone_key},{cur_node_name_str},{cur_node_phase_str}")


def calc_segment_loading():
    _, seg_loading_p_dict, seg_loading_q_dict = test_mapping_zone_info()
    total_load_p = 0
    total_load_q = 0
    for key, val in seg_loading_p_dict.items():
        total_load_p += val

        val_q = seg_loading_q_dict[key]
        total_load_q += val_q

        print(f"Segment {key}: {val / 1e3} (kW), {val_q / 1e3} (kVAR)")

    print(
        f"Total load of all segments: {total_load_p / 1e3}(kW), {total_load_q / 1e3}(kVAR)"
    )


def test_read_inv_names():
    # ==Parameters
    inv_glm_path_fn = r"D:\Duke_UC3_S1_Tap12_[with MG][Clean][LessLoad]\SolarPV.glm"

    # ==Test & Demo
    p = GlmParser()
    p.read_inv_names(inv_glm_path_fn)

    print(p.all_invs_names_list)


def test_set_inverters():
    # ==Parameters
    # csv_qout_fp = r'D:\#Github\duke_te2\UC1SC1_InitTopo_dv_150v'
    csv_qout_fp = r'D:\#Github\duke_te2\UC1SC1_MidTopo_dv_5v'
    csv_qout_fn = r'solution_short.csv'
    csv_qout_fpn = os.path.join(csv_qout_fp, csv_qout_fn)

    # glm_inv_src_fp = r'D:\test glms_UC1SC1_InitTopo'
    glm_inv_src_fp = r'D:\test glms_UC1SC1_MidTopo'
    glm_inv_src_fn = r'Copy_SolarPV.glm'

    # glm_inv_dst_fp = r'D:\test glms_UC1SC1_InitTopo'
    glm_inv_dst_fp = r'D:\test glms_UC1SC1_MidTopo'
    # glm_inv_dst_fn = r'SolarPV_AfterSwtOpt.glm'
    glm_inv_dst_fn = r'SolarPV_AfterSwtOpt_MidTopo.glm'

    # ==Test & Demo
    p = GlmParser()
    p.set_inverters(csv_qout_fpn, glm_inv_src_fp, glm_inv_src_fn, glm_inv_dst_fp, glm_inv_dst_fn)


def test_adjust_triplex_nodes():
    # ==Parameters
    # ld_mult = 0.5 / 0.7
    # ld_mult = 0.6 / 0.7
    # ld_mult = 1
    # ld_mult = 0.8 / 0.7
    # ld_mult = 0.9 / 0.7
    # ld_mult = 1.0 / 0.7
    # ld_mult = 1.1 / 0.7
    ld_mult = 1.2 / 0.7

    ld_type = 'p_to_z'

    triplex_node_glm_path_fn = r"triplex_nodes.glm"
    adj_triplex_node_glm_path_fn = fr"triplex_nodes_{round(ld_mult * 100)}_{ld_type}.glm"

    # ==Test & Demo
    p = GlmParser()

    p.adjust_triplex_nodes(triplex_node_glm_path_fn, adj_triplex_node_glm_path_fn, ld_mult, ld_type)


def test_add_parallel_cables():
    # ==Parameters
    from scl_pcs import scl_pcs_names_list

    tar_glm_fpn = r'D:\#Temp\636.glm'
    pcs_glm_fpn = r'D:\#Temp\636_pcs.glm'

    # ==Test & Typical Usage Example
    p = GlmParser()
    p.add_parallel_cables(scl_pcs_names_list, tar_glm_fpn, pcs_glm_fpn)


def test_add_triplex_loads():
    # ==Parameters
    ld_type = 'p_to_zip'

    # --ld_mult
    # ld_mult = 0.5 / 0.7
    # ld_mult = 0.6 / 0.7
    ld_mult = 1
    # ld_mult = 0.8 / 0.7
    # ld_mult = 0.9 / 0.7
    # ld_mult = 1.0 / 0.7
    # ld_mult = 1.1 / 0.7
    # ld_mult = 1.2 / 0.7

    # --PF
    p_pf = 0.9
    i_pf = 0.9
    z_pf = 0.9

    # --ZIP Percentages
    p_pct = 1 / 3
    i_pct = 1 / 3
    z_pct = 1.0 - p_pct - i_pct

    # == Files
    triplex_node_glm_path_fn = r"triplex_nodes.glm"
    adj_triplex_node_glm_path_fn = fr"triplex_nodes_{round(ld_mult * 100)}_{ld_type}.glm"

    # ==Test & Demo
    p = GlmParser()

    p.add_triplex_loads(triplex_node_glm_path_fn, adj_triplex_node_glm_path_fn, ld_mult, ld_type,
                        p_pf, i_pf, z_pf, p_pct, i_pct, z_pct)


def test_read_content_node_3ph():
    # ==Parameters
    main_glm_path_fn = r"D:\Inverters\IEEE_8500node_NR.glm"

    # ==Test & Demo
    p = GlmParser()
    p.read_content_node_3ph(main_glm_path_fn)

    print(p.all_nodes_names_list[0:2])
    print(len(p.all_nodes_list))

    print(p.all_nodes_names_3ph_list[0:2])
    print(len(p.all_nodes_phases_3ph_dict))

    print(p.all_nodes_names_3ph_list)


def test_add_inverter_dyn():
    # ==Parameters
    node_name_list = ['"_HVMV_Sub_LSB"', '"D6413567-3_INT"', '"221-311359"', '"221-308718"', '"P862322"',
                      '"221-242079"', '"M1108398"', '"M1108387"', '"M1089185"', '"P850072"', '"M1089183"', '"M1209774"',
                      '"198-5320"', '"M1108381"', '"M1186052"', '"P850080"', '"221-311905"', '"221-312488"',
                      '"P850083"', '"221-311609"', '"M1009651"', '"L2925506"', '"M1069418"', '"L3104136"', '"M1069504"',
                      '"M1026855"', '"M1069517"', '"M1047497"', '"L2745848"', '"N1136667"', '"L2876798"', '"L3197646"',
                      '"M1047507"', '"M1069500"', '"M1026858"', '"L3181545"', '"L2991939"', '"N1138603"', '"M1047528"',
                      '"M1047574"', '"M1047577"', '"M1047592"', '"M1047484"', '"L2822871"', '"L2935549"', '"Q14411"',
                      '"M1026905"', '"M1047548"', '"L2879070"', '"M1069424"', '"L3143999"', '"N1139260"', '"L2820531"',
                      '"N1138598"', '"M1047490"', '"L3235254"', '"N1144668"', '"L2766741"', '"M1047486"', '"L2822877"',
                      '"M1069483"', '"L3103822"', '"M1047483"', '"Q14733"', '"M1047534"', '"N1138599"', '"190-8593"',
                      '"M1026861"', '"M1069420"', '"L2841632"', '"M1069311"', '"L3010560"', '"196-29519"', '"M1069350"',
                      '"N1136666"', '"M1026896"', '"M1026854"', '"L3120484"', '"N1138604"', '"N1138607"', '"M1069514"',
                      '"M1069382"', '"L3066815"', '"M1047568"', '"L3160109"', '"Q16642_CAP"', '"N1136657"',
                      '"M1069464"', '"M1069481"', '"M1047480"', '"M3768670"', '"N1136367"', '"L2858166"', '"L3235257"',
                      '"M1047552"', '"M1069468"', '"M1069417"', '"L3216367"', '"M1069461"', '"L3122821"', '"M1026830"',
                      '"M1069515"', '"L3159448"', '"M1026902"', '"M1047522"', '"Q14413"', '"L2673308"', '"M1069419"',
                      '"L2766749"', '"N1137992"', '"196-29520"', '"M1026852"', '"M1026872"', '"L2822872"', '"L3125349"',
                      '"M1069376"', '"M3763619"', '"L2691967"', '"M1069438"', '"M1047485"', '"N1136669"', '"R42246"',
                      '"M1047558"', '"M1026886"', '"M1047526"', '"L3216345"', '"M4113347"', '"N1136663"', '"M1047550"',
                      '"M4277837"', '"M1069457"', '"M1026851"', '"M1069411"', '"P827525"', '"M1069428"', '"L2726973"',
                      '"N1138597"', '"M1047521"', '"M1186065"', '"M1026808"', '"M1026744"', '"M1026690"', '"L3139366"',
                      '"M1026709"', '"M1026997"', '"N1140521"', '"M1026769"', '"M1026807"', '"M1026970"', '"L2879078"',
                      '"M1027000"', '"M1026960"', '"M1026701"', '"M4362177"', '"M1026697"', '"M1026984"', '"M1089143"',
                      '"M1026726"', '"L3048221"', '"M1026702"', '"N1140817"', '"L3254237"', '"M1026977"', '"M1026681"',
                      '"M1026708"', '"M1089145"', '"L2851933"', '"L3254217"', '"N1140828"', '"M3763624"', '"M1026978"',
                      '"M1026986"', '"M1026706"', '"L3207907"', '"M1186061"', '"M1026979"', '"P829977"', '"L3232902"',
                      '"M1108490"', '"M1069549"', '"E182729"', '"M1125960"', '"L2842383"', '"M1166376"', '"M1209772"',
                      '"N1142100"', '"L3067478"', '"M1069496"', '"R18245"', '"M1069564"', '"N1134480"', '"M1125934"',
                      '"M1069556"', '"L2973791"', '"M1209788"', '"N1136366"', '"M1125990"', '"Q14734"', '"M1209787"',
                      '"M1125994"', '"L3030197"', '"E206217"', '"M1069499"', '"M1142860"', '"L2786266"', '"M1089215"',
                      '"M1209797"', '"M1069503"', '"M1069509"', '"N1142099"', '"M1125952"', '"L3081380"', '"E183493"',
                      '"M1089203"', '"L2973833"', '"N1136363"', '"L2730163"', '"M1108534"', '"M1166373"', '"M1125962"',
                      '"M1209791"', '"M1125917"', '"M1125914"', '"E206211"', '"M1089197"', '"L2767341"', '"M1166374"',
                      '"M1142843"', '"R20703"', '"L2692655"', '"M1125919"', '"L3123509"', '"M1125987"', '"M1108500"',
                      '"M1142875"', '"M1142828"', '"M1149235"', '"N1134478"', '"Q1301"', '"M1108315"', '"N1142103"',
                      '"M1142818"', '"M1186064"', '"L2955047"', '"M1166368"', '"L3179650"', '"M1166375"', '"M1149236"',
                      '"M1125902"', '"M1142863"', '"M1209805"', '"L3104830"', '"M1142865"', '"M3032977"', '"L3123452"',
                      '"M1209776"', '"M1125997"', '"M1125944"', '"L2674047"', '"R20185"', '"M1125968"', '"L2748144"',
                      '"M1125947"', '"M1186067"', '"N1136365"', '"M1089202"', '"L3030204"', '"L2992624"', '"196-31070"',
                      '"M1069495"', '"L3179646"', '"M1069513"', '"190-8581"', '"M1142839"', '"M1149233"', '"L3214069"',
                      '"M1108493"', '"L2823611"', '"M3032980"', '"E184626"', '"M1186063"', '"N1134476"', '"M1209775"',
                      '"L3118855"', '"N1136354"', '"M1142869"', '"Q16483_CAP"', '"L2933135"', '"L2955074"', '"E183473"',
                      '"L2955077"', '"N1134475"', '"M1089213"', '"M1108311"', '"M1166377"', '"M1089201"', '"M1142874"',
                      '"L3160098"', '"L3066814"', '"M1125989"', '"M1069519"', '"M1108526"', '"M1108535"', '"E192201"',
                      '"M1089196"', '"M1089198"', '"R42247"', '"M1125969"', '"M1186072"', '"L2859403"', '"M1142832"',
                      '"L2917328"', '"M1209790"', '"N1142104"', '"M1069518"', '"L2861218"', '"N1134477"', '"L2879773"',
                      '"M1166366"', '"M1142851"', '"L3142049"', '"M1089199"', '"L2730106"', '"L3231255"', '"M1069498"',
                      '"L3030199"', '"M1069516"', '"L3160872"', '"L2766738"', '"L2692633"', '"M1125976"', '"L2955081"',
                      '"M1108484"', '"226-23751"', '"E193509"', '"L2842384"', '"L2823592_CAP"', '"L3214071"',
                      '"M1069497"', '"M1108298"', '"M1142819"', '"N1136356"', '"M1069533"', '"N1134479"', '"M1142868"',
                      '"M1069505"', '"M1108529"', '"N1134474"', '"M1108532"', '"L2674027"', '"L2711226"', '"M1209800"',
                      '"M4122658"', '"M1108505"', '"M1142871"', '"N1144663"', '"M1089207"', '"L3160865"', '"M1209807"',
                      '"HVMV_Sub_48332"', '"M1209817"', '"L2801909"', '"M1209811"', '"E192860"', '"M1209814"',
                      '"L3048965"', '"L2804256"', '"M1026954"', '"M1027002"', '"M1027039"', '"M1026927"', '"M1027011"',
                      '"M1026926"', '"L2916608"', '"M1026950"', '"M1026907"', '"L2673313"', '"196-29518"', '"M1027013"',
                      '"M1027023"', '"M1026920"', '"L3122816"', '"M1026915"', '"L3160103"', '"M1027043"', '"L3254213"',
                      '"M1089141"', '"M1026329"', '"M1142821"', '"L2973155"', '"E183472"', '"L3784018"', '"L3139121"',
                      '"M1047593"', '"L2841633"', '"N1136028"', '"N1136996"', '"N1137986"', '"M1047615"', '"L3216368"',
                      '"N1136671"', '"M1069154"', '"M1069156"', '"M1026797"', '"N1145954"', '"M1047713"', '"P827580"',
                      '"L2876814"', '"M1026800"', '"N1136670"', '"M1047325"', '"M1026724"', '"P827514"', '"N1140823"',
                      '"M1142815"', '"L2766718"', '"M1047572"', '"M1047300"', '"M1047303"', '"M1047580"', '"R18243"',
                      '"196-36167"', '"M1026891"', '"L2860491"', '"N1137005"', '"L3027157"', '"196-35813"',
                      '"M1142814"', '"M1047737"', '"L3216123"', '"L2691954"', '"M1026760"', '"L2841635"', '"L3010556"',
                      '"L2841621"', '"M1108295"', '"M1026357"', '"L3122849"', '"L3235275"', '"M1026866"', '"L2895449"',
                      '"M1026354"', '"L2763153"', '"E182732"', '"N1136658"', '"L3254218"', '"N1140825"', '"N1139552"',
                      '"M1026876"', '"N1140519"', '"L3085398"', '"L2689691"', '"M1047503"', '"L3029498"', '"P828362"',
                      '"P827527"', '"M1047293"', '"M1047515"', '"M1009763"', '"L3160102"', '"L2860490"', '"M1026844"',
                      '"M1047513"', '"M1047732"', '"L2896685"', '"M1047339"', '"M1108395"', '"M1026829"', '"N1140827"',
                      '"M1026330"', '"L2991914"', '"F739845"', '"L2822869"', '"M1047720"', '"M1026700"', '"M1026356"',
                      '"M1026796"', '"M1047299"', '"M1026834"', '"M1108286"', '"L3254238"', '"Q14412"', '"L2897780"',
                      '"M1069155"', '"R18242"', '"L3027116"', '"M1009650"', '"M1009770"', '"M1089131"', '"R18241"',
                      '"M1026795"', '"N1140818"', '"L3029502"', '"M1026695"', '"L2936271"', '"L2897777"', '"L3104125"',
                      '"L2673346"', '"L2933164"', '"E182748"', '"M1047316"', '"M1026786"', '"M1089118"', '"L2688692"',
                      '"L2973167"', '"E206209"', '"N1139551"', '"M1089115"', '"Q14404"', '"M1047724"', '"L2876797"',
                      '"E182724"', '"M1026785"', '"M1089120"', '"L3179678"', '"M1069136"', '"L3048206"', '"L3216370"',
                      '"M1047312"', '"L2936279"', '"M1069169"', '"190-7361"', '"M1089119"', '"L3235256"', '"L2688693"',
                      '"M1047292"', '"M1069175"', '"M1047309"', '"M1026774"', '"L3160107"', '"N1144665"', '"N1139547"',
                      '"M1026347"', '"L3235255"', '"N1139541"', '"N1137960"', '"M1047722"', '"M1026780"', '"M1026783"',
                      '"P829965"', '"L2804249"', '"N1140819"', '"M1047324"', '"L3197630"', '"L3011298"', '"M1047649"',
                      '"L2822866"', '"L2691952"', '"L2952003"', '"M1009784"', '"L2973162"', '"M1047346"', '"L2898495"',
                      '"L3104134"', '"M1069184"', '"M1009807"', '"M1026333"', '"M1026670"', '"M1047345"', '"L2991922"',
                      '"M1026655"', '"L2673319"', '"Q14414"', '"M1026331"', '"M3036170"', '"M1026662"', '"M1026658"',
                      '"M1009805"', '"M1069189"', '"P901945"', '"M1047699"', '"L2767340"', '"M1069135"', '"L2989566"',
                      '"L2973153"', '"M1108378"', '"M1089112"', '"M1047688"', '"L2935554"', '"M1026824"', '"M1125988"',
                      '"M1026341"', '"N1136998"', '"M1047672"', '"M1047669"', '"L3195356"', '"M1108380"', '"M1108379"',
                      '"D6023352-1_INT"', '"D6198039-1_INT"', '"D5534969-2_INT"', '"D6049825-1_INT"',
                      '"D5958866-1_INT"', '"D5860423-3_INT"', '"D5513564-1_INT"', '"D6138608-3_INT"',
                      '"D5587291-3_INT"', '"D5535139-1_INT"', '"D6290228-6_INT"', '"D6108141-1_INT"',
                      '"D5534970-1_INT"', '"D5710794-3_INT"', '"D5799561-2_INT"', '"D5861005-2_INT"',
                      '"D5926308-3_INT"', '"D5867591-1_INT"', '"D5563942-4_INT"', '"D5956499-2_INT"',
                      '"D5835167-6_INT"', '"D5502543-2_INT"', '"D5686080-1_INT"', '"D5682346-3_INT"', '"M3763618"',
                      '"M1027055"', '"M3037441"', '"M1047763"', '"M1047566"', '"Q16642"', '"L2823592"', '"M1142810"',
                      '"M1026328"', '"M3037449"', '"Q16483"', '"L3234149"', '"L2803199"', '"L3011293"', '"L2990826"',
                      '"L3215203"', '"L2915542"', '"regxfmr_190-8593"', '"M1069310"', '"L2814529"', '"L2729414"',
                      '"L3235258"', '"L3215385"', '"L3047289"', '"E192258"', '"E182746"', '"L2916620"',
                      '"regxfmr_190-8581"', '"E182733"', '"293471"', '"L2728247"', '"E203026"', '"regxfmr_190-7361"',
                      '"L2730149"']
    reg_fm_to_node_list = ['"regxfmr_190-8593"', '"190-8593"', '"regxfmr_190-8581"', '"190-8581"', '"regxfmr_190-7361"',
                           '"190-7361"']
    cap_par_node_list = ['"R42246"', '"R42247"', '"R20185"', '"R18242"']
    tar_glm_fpn = 'D:\Inverters\inv_dyn_{}_{}.glm'

    # ==Prep
    print(len(node_name_list))
    for cur_node_str in reg_fm_to_node_list + cap_par_node_list:
        if cur_node_str in node_name_list:
            node_name_list.remove(cur_node_str)
    print(len(node_name_list))

    # ==Test & Demo
    for cur_pct_gflw in [0.0, 0.5, 1.0]:
        for cur_num_inv in [300, 400, 500]:
            p = GlmParser()
            p.add_inverter_dyn(tar_glm_fpn.format(cur_num_inv, cur_pct_gflw), node_name_list,
                               pct_gflw=cur_pct_gflw, num_inv=cur_num_inv)


if __name__ == "__main__":
    # test_add_ufls_gfas()
    # test_separate_load_objs()
    # test_adjust_load_amount()
    # test_read_content_load()
    # test_read_content_node()
    # test_read_zone_info()

    # calc_segment_loading()

    # test_pick_node_from_segments()
    # test_load_zone_info()

    # test_read_inv_names()
    # test_set_inverters()

    # test_adjust_triplex_nodes()

    # test_add_parallel_cables()

    # test_add_triplex_loads()

    # test_read_content_load()

    # test_read_content_node_3ph()

    test_add_inverter_dyn()
