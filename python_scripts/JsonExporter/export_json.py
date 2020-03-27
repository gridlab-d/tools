# ***************************************
# Author: Jing Xie
# Created Date: 2020-3
# Updated Date: 2020-3-23
# Email: jing.xie@pnnl.gov
# ***************************************

import os.path
import json
import warnings


class JsonExporter:
    """Export the .json file(s)"""

    def __init__(
        self,
        cc_ep_pref="ctrl_",
        ns3_filters_pref="filter_",
        sort_flag=False,
        indent_val=4,
    ):
        """Init the settings
        """
        # == for json.dump()
        self.json_dump_sort_key = sort_flag
        self.json_dump_indent_val = indent_val

        # == for CC json file
        self.param_cc_ep_pref = cc_ep_pref
        self.param_cc_ep_type = "string"
        self.param_cc_ep_global = False

        # == for ns3 json file
        # ~~ filters
        self.param_ns3_filters_pref = ns3_filters_pref
        self.param_ns3_filter_oper = "reroute"
        self.param_ns3_filter_prop_name = "newdestination"

        # ~~ endpoints
        self.param_ns3_ep_info = "0"
        self.param_ns3_ep_global = False

        # == for gld json file
        self.gld_ep_all_list = []
        self.gld_all_key_list = []
        self.param_gld_ep_type = "string"
        self.param_gld_ep_global = False

    def dump_json(self, json_path_fn, json_data_dic):
        """Dump the data dictionary into a json file
        """
        with open(json_path_fn, "w") as hf_json:
            json.dump(
                json_data_dic,
                hf_json,
                sort_keys=self.json_dump_sort_key,
                indent=self.json_dump_indent_val,
            )

    def get_gld_endpoints(self, gld_comp_list, gld_ep_obj_dic, ep_property):
        """
        Static method for handling different types of endpoints
        """
        gld_ep_list = []
        for cur_comp_str in gld_comp_list:
            cur_ep_dic = {}

            # --other properties
            cur_ep_dic["global"] = self.param_gld_ep_global
            cur_ep_dic["name"] = cur_comp_str
            cur_ep_dic["type"] = self.param_gld_ep_type

            # --'info' property
            if cur_comp_str in gld_ep_obj_dic:
                cur_ep_info_obj = gld_ep_obj_dic[cur_comp_str]
            else:
                cur_ep_info_obj = cur_comp_str
            cur_ep_info_dic = {"object": cur_ep_info_obj, "property": ep_property}
            cur_ep_dic["info"] = json.dumps(cur_ep_info_dic)

            # --assemble
            gld_ep_list.append(cur_ep_dic)

        self.gld_ep_all_list += gld_ep_list
        self.gld_all_key_list += gld_comp_list

    def update_param_gld_endpoints(self, gld_ep_type="string", gld_ep_global=False):
        self.param_gld_ep_type = gld_ep_type
        self.param_gld_ep_global = gld_ep_global

    def update_param_cc_endpoints(
        self, cc_ep_pref, cc_ep_type="string", cc_ep_global=False
    ):
        self.param_cc_ep_pref = cc_ep_pref
        self.param_cc_ep_type = cc_ep_type
        self.param_cc_ep_global = cc_ep_global

    def get_cc_endpoints(self):
        """
        Member method for adding a controller with respect to each endpoint defined in the GLD json file
        """
        # ~~controllers
        self.cc_ep_list = []
        self.cc_list_all_key = []
        for cur_gld_ep_name in self.gld_list_all_key:
            cur_cc_ep_dict = {}

            cur_cc_ep_dict["global"] = self.param_cc_ep_global
            cur_cc_ep_dict["name"] = self.param_cc_ep_pref + cur_gld_ep_name
            cur_cc_ep_dict[
                "destination"
            ] = f"{self.gld_json_config_name}/{cur_gld_ep_name}"
            cur_cc_ep_dict["type"] = self.param_cc_ep_type

            self.cc_ep_list.append(cur_cc_ep_dict)
            self.cc_list_all_key.append(cur_cc_ep_dict["name"])

    def update_param_ns3_endpoints(self, ns3_ep_info, ns3_ep_global):
        self.param_ns3_ep_info = ns3_ep_info
        self.param_ns3_ep_global = ns3_ep_global

    def get_ns3_endpoints(self):
        """
        """
        self.ns3_ep_list = []

        # --adding the endpoints defined in the GLD json file
        for cur_gld_ep_name in self.gld_list_all_key:
            cur_gld_ep_dict = {}
            cur_gld_ep_dict["name"] = f"{self.gld_json_config_name}/{cur_gld_ep_name}"
            cur_gld_ep_dict["info"] = self.param_ns3_ep_info
            cur_gld_ep_dict["global"] = self.param_ns3_ep_global

            self.ns3_ep_list.append(cur_gld_ep_dict)

        # --adding the endpoints defined in the CC json file
        for cur_cc_ep_name in self.cc_list_all_key:
            cur_cc_ep_dict = {}
            cur_cc_ep_dict["name"] = f"{self.cc_json_config_name}/{cur_cc_ep_name}"
            cur_cc_ep_dict["info"] = self.param_ns3_ep_info
            cur_cc_ep_dict["global"] = self.param_ns3_ep_global

            self.ns3_ep_list.append(cur_cc_ep_dict)

    def update_param_ns3_filters(
        self,
        ns3_filters_pref,
        ns3_filter_oper="reroute",
        ns3_filter_prop_name="newdestination",
    ):
        self.param_ns3_filters_pref = ns3_filters_pref
        self.param_ns3_filter_oper = ns3_filter_oper
        self.param_ns3_filter_prop_name = ns3_filter_prop_name

    def get_ns3_sub_filters(self, list_all_key, json_config_name):
        # --adding filters
        for cur_gld_ep_name in list_all_key:
            cur_ns3_filter_dict = {}
            cur_ns3_filter_dict[
                "name"
            ] = f"{self.param_ns3_filters_pref}{json_config_name}_{cur_gld_ep_name}"
            cur_ns3_filter_dict["sourcetargets"] = [
                f"{json_config_name}/{cur_gld_ep_name}"
            ]
            cur_ns3_filter_dict["operation"] = self.param_ns3_filter_oper

            # ~~property dict
            cur_ns3_filter_prop_dict = {}
            cur_ns3_filter_prop_dict["name"] = self.param_ns3_filter_prop_name
            cur_ns3_filter_prop_dict[
                "value"
            ] = f"{self.ns3_json_config_name}/{json_config_name}/{cur_gld_ep_name}"

            cur_ns3_filter_dict["properties"] = cur_ns3_filter_prop_dict

            # ~~assemble
            self.ns3_filters_list.append(cur_ns3_filter_dict)

    def get_ns3_filters(self):
        self.ns3_filters_list = []

        self.get_ns3_sub_filters(self.gld_list_all_key, self.gld_json_config_name)
        self.get_ns3_sub_filters(self.cc_list_all_key, self.cc_json_config_name)

    def export_gld_json(self, output_json_path_fn, json_data_settings_dic):
        # --record
        self.gld_json_config_name = json_data_settings_dic["name"]

        # --prepare
        self.gld_data = {}
        self.gld_data.update(json_data_settings_dic)
        self.gld_data["endpoints"] = self.gld_ep_all_list

        # --dump
        self.dump_json(output_json_path_fn, self.gld_data)

        # --record
        self.gld_list_all_key = self.gld_all_key_list

    def export_cc_json(self, output_json_path_fn, json_data_settings_dic):
        # --record
        self.cc_json_config_name = json_data_settings_dic["name"]

        # --prepare
        self.cc_data = {}
        self.cc_data.update(json_data_settings_dic)

        # --end points
        # p.update_param_cc_endpoints(cc_ep_pref='ctr_') # left here as a demo
        p.get_cc_endpoints()

        if not hasattr(self, "cc_ep_list"):
            warnings.warn(
                "The attribute 'cc_ep_list' is not initialized. An empty list is used."
            )
            self.cc_ep_list = []
        self.cc_data["endpoints"] = self.cc_ep_list

        # --dump
        self.dump_json(output_json_path_fn, self.cc_data)

    def export_ns3_json(self, output_json_path_fn, json_data_settings_dic):
        # --record
        self.ns3_json_config_name = json_data_settings_dic["name"]

        # --prepare
        self.ns3_data = {}
        self.ns3_data.update(json_data_settings_dic)

        # --end points
        self.get_ns3_endpoints()
        self.ns3_data["endpoints"] = self.ns3_ep_list

        # --filters
        self.get_ns3_filters()
        self.ns3_data["filters"] = self.ns3_filters_list

        # --dump
        self.dump_json(output_json_path_fn, self.ns3_data)


def test_export_gld_json(p):
    # ==Parameters
    # --file path & name
    output_json_path_fn = "Duke_gld_config.json"

    # --file configs
    json_data_settings_dic = {
        "name": "GLD",
        "coreType": "zmq",
        "loglevel": 7,
        "period": 1e-2,
        "wait_for_current_time_update": True,
    }
    # --end Points
    # ~~switches
    ep_property = "status"

    list_cbs_key = ["CB" + str(x) for x in range(1, 5)]
    list_cbs_val = ["CB_" + str(x) for x in range(1, 5)]
    ep_cbs_obj_dic = dict(zip(list_cbs_key, list_cbs_val))
    p.get_gld_endpoints(list_cbs_key, ep_cbs_obj_dic, ep_property)

    list_rcls_key = ["RCL" + str(x) for x in range(1, 13)]
    ep_rcls_obj_dic = {
        "RCL1": "RCL1_53547349_1207",
        "RCL2": "RCL2_53547361_1207",
        "RCL5": "RCL5_410845731_1209",
        "RCL7": "RCL7_39067965_1210",
        "RCL8": "RCL8_164887213_1210",
        "RCL9": "RCL9_39057658_1209",
        "RCL11": "RCL11_616009826_1210",
        "RCL12": "RCL12_164887203_1212",
    }
    p.get_gld_endpoints(list_rcls_key, ep_rcls_obj_dic, ep_property)

    # ~~inverters
    ep_inv_property = "Q_Out"

    # list_invs_key = ['INV_Inv_S2_n256860617_1207']
    # list_invs_val = ['Inv_S2_n256860617_1207']
    list_invs_val = [
        "Inv_S2_n256860617_1207",
        "Inv_S2_n264462704_1207",
        "Inv_S2_n256860535_1207",
        "Inv_S2_n259382785_1207",
        "Inv_S2_n865810615_1207",
        "Inv_S1_n259567965_1207",
        "Inv_S1_n865808803_1207",
        "Inv_S1_n866083429_1207",
        "Inv_S1_n256851477_1207",
        "Inv_S6_n256886665_1209",
        "Inv_S6_n865810391_1209",
        "Inv_S6_n256886462_1209",
        "Inv_S6_n256886547_1209",
        "Inv_S6_n259333327_1209",
        "Inv_S5_n259247296_1209",
        "Inv_S5_n256886285_1209",
        "Inv_S5_n259100323_1209",
        "Inv_S5_n256886226_1209",
        "Inv_S5_n256886268_1209",
        "Inv_S4_n256886150_1209",
        "Inv_S4_n256886199_1209",
        "Inv_S4_n256877737_R_1209",
        "Inv_S4_n256886165_R_1209",
        "Inv_S4_n260450711_1209",
        "Inv_S3_n260448238_1209",
        "Inv_S3_n256877623_1209",
        "Inv_S3_n262957200_1209",
        "Inv_S3_n256877677_1209",
        "Inv_S3_n259093065_1209",
        "Inv_S8_n259113136_1210",
        "Inv_S8_n262965849_1210",
        "Inv_S8_n256891983_1210",
        "Inv_S8_n264463390_1210",
        "Inv_S8_n259382647_1210",
        "Inv_S7_n256886685_1210",
        "Inv_S7_n260450817_1210",
        "Inv_S7_n256886863_1210",
        "Inv_S7_n256777666_1210",
        "Inv_S7_n256886749_1210",
        "Inv_S9_n261128590_1210",
        "Inv_S9_n256904456_1210",
        "Inv_S9_n256904451_1210",
        "Inv_S9_n256904446_1210",
        "Inv_S9_n263855034_1210",
        "Inv_S10_n256824095_1212",
        "Inv_S10_n256834332_1212",
        "Inv_S10_n398510026_1212",
        "Inv_S10_n865809671_1212",
        "Inv_S10_n614183714_1212",
        "Inv_S11_n256817812_1212",
        "Inv_S11_n256834463_1212",
        "Inv_S11_n256817837_1212",
        "Inv_S11_n256823986_1212",
        "Inv_S11_n256834462_1212",
    ]
    list_invs_key = ["INV_" + str(x) for x in list_invs_val]
    ep_invs_obj_dic = dict(zip(list_invs_key, list_invs_val))
    p.get_gld_endpoints(list_invs_key, ep_invs_obj_dic, ep_inv_property)

    # ==Export
    p.export_gld_json(output_json_path_fn, json_data_settings_dic)


def test_export_cc_json(p):
    # ==Parameters
    output_json_path_fn = "Duke_cc_config.json"

    # --file configs
    json_data_settings_dic = {
        "name": "CC",
        "coreType": "zmq",
        "loglevel": 7,
        "period": 1e-2,
        "wait_for_current_time_update": True,
    }

    # ==Export
    p.export_cc_json(output_json_path_fn, json_data_settings_dic)


def test_export_ns3_json(p):
    # ==Parameters
    output_json_path_fn = "Duke_ns3_config.json"

    # --file configs
    json_data_settings_dic = {
        "name": "ns3",
        "coreType": "zmq",
        "loglevel": 7,
        "period": 1e-9,
        "wait_for_current_time_update": True,
    }

    # ==Export
    p.export_ns3_json(output_json_path_fn, json_data_settings_dic)


if __name__ == "__main__":
    p = JsonExporter()
    test_export_gld_json(p)
    test_export_cc_json(p)
    test_export_ns3_json(p)
