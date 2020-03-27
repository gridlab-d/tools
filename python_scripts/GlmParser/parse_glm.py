# ***************************************
# Author: Jing Xie
# Created Date: 2019-10
# Updated Date: 2020-1-22
# Email: jing.xie@pnnl.gov
# ***************************************

import re
import os.path
import csv

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
        self.all_loads_list = []
        self.all_loads_p_list = []
        self.all_loads_q_list = []

        self.all_nodes_list = []

        # ==GLM SYN
        self.re_glm_syn_comm = "\/\/.*\n"
        self.re_glm_syn_mty_lns = r"^(?:[\t ]*(?:\r?\n|\r))+"

        self.re_glm_syn_obj_load = r"object\s*load.*?{.*?}\s*;*"
        self.re_glm_syn_obj_node = r"object\s*node.*?{.*?}\s*;*"
        # self.re_glm_syn_obj_x = r'object\s*{}.*?{.*?}\s*;*'
        self.re_glm_syn_obj_inv = r"object\s*inverter.*?{.*?}\s*;*"

        self.re_glm_attr_tpl_str = ".*{}\s*(.*?);"

        # ==GLM OBJ TPL for EXPORT
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

    def clean_load_buffer(self):
        self.all_loads_list = []
        self.all_loads_p_list = []
        self.all_loads_q_list = []

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
            cur_ld_obj_sabc[0] = self.extract_attr("constant_power_A", cur_obj_str)
            cur_ld_obj_sabc[1] = re.findall(
                r".*constant_power_B\s*(.*?);", cur_obj_str, flags=re.DOTALL
            )
            cur_ld_obj_sabc[2] = re.findall(
                r".*constant_power_C\s*(.*?);", cur_obj_str, flags=re.DOTALL
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

    def disp_load_info(self):
        print(
            f"Total Load: {self.all_loads_p_sum/1e3} (kW), {self.all_loads_q_sum/1e3} (kVAR)"
        )

    def read_content_node(self, filename):
        str_file_woc = self.import_file(filename)
        self.parse_node(str_file_woc)

    def read_content_load(self, filename):
        """This func is added as an extra layer for flexible extension"""
        str_file_woc = self.import_file(filename)
        self.parse_load(str_file_woc)
        self.disp_load_info()

    def read_content_triload(self, filename):
        str_file_woc = self.import_file(filename)
        self.parse_triload(str_file_woc)

    def read_inv_names(self, filename):
        str_file_woc = self.import_file(filename)
        self.parse_inv(str_file_woc)

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
        print(f"Total Load after Adjustment: {p_ratio*self.all_loads_p_sum/1e3} (kW)")
        print(f"Power Factor: {tgt_pf}\n")

        self.all_adj_loads_list = []
        out_adj_load_str = ""
        for cur_obj_str in self.all_loads_list:
            cur_new_obj_str = self.update_pq(cur_obj_str, p_ratio, tgt_pf)
            self.all_adj_loads_list.append(cur_new_obj_str)

            out_adj_load_str += self.obj_load_tpl_str.format(cur_new_obj_str, "")

        self.export_glm(adj_load_glm_path_fn, out_adj_load_str)

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
    load_glm_path_fn = r"D:\UC3_S1_Tap12_[with MG][Clean][LessLoad]\duke_loads_adj.glm"

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

        print(f"Segment {key}: {val/1e3} (kW), {val_q/1e3} (kVAR)")

    print(
        f"Total load of all segments: {total_load_p/1e3}(kW), {total_load_q/1e3}(kVAR)"
    )


def test_read_inv_names():
    # ==Parameters
    inv_glm_path_fn = r"D:\Duke_UC3_S1_Tap12_[with MG][Clean][LessLoad]\SolarPV.glm"

    # ==Test & Demo
    p = GlmParser()
    p.read_inv_names(inv_glm_path_fn)

    print(p.all_invs_names_list)


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

    test_read_inv_names()
