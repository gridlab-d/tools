# ***************************************
# Author: Jing Xie
# Created Date: 2020-8-6
# Updated Date: 2020-8-6
# Email: jing.xie@pnnl.gov
# ***************************************

import re
import math

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

def calc_volt_mag_diff(rcl_name_str, a, b, c, d):
    a_cp = [parse_volt_phasor(cur_item) for cur_item in a]
    b_cp = [parse_volt_phasor(cur_item) for cur_item in b]
    c_cp = [parse_volt_phasor(cur_item) for cur_item in c]
    d_cp = [parse_volt_phasor(cur_item) for cur_item in d]

    from_node_cmp_list = [abs(cur_a) - abs(cur_c) for cur_a, cur_c in zip(a_cp, c_cp)]
    print(f'From node of "{rcl_name_str}" voltage magnitude differences (a, b, c phases): {from_node_cmp_list}')

    to_node_cmp_list = [abs(cur_b) - abs(cur_d) for cur_b, cur_d in zip(b_cp, d_cp)]
    print(to_node_cmp_list)

    # print(a_cp)
    # print(b_cp)
    no_pv_rcl_fmto_list = [abs(cur_a) - abs(cur_b) for cur_a, cur_b in zip(a_cp, b_cp)]
    print(no_pv_rcl_fmto_list)

    with_pv_rcl_fmto_list = [abs(cur_c) - abs(cur_d) for cur_c, cur_d in zip(c_cp, d_cp)]
    print(with_pv_rcl_fmto_list)

def calc_init_topo():
    rcl11_from_node_init_topo_noPV = ['+6972.13-4.40283d', '+7218.33-123.853d', '+7248.63+117.416d']
    rcl11_to_node_init_topo_noPV = ['+6908.24-4.37541d', '+7083.97-122.137d', '+6969.93+117.186d']

    rcl11_from_node_init_topo_PV = ['+6972.13-4.40283d', '+7218.33-123.853d', '+7248.63+117.416d']
    rcl11_to_node_init_topo_PV = ['+7067.5-4.74855d', '+7219.6-122.583d', '+7116.98+117.004d']

    calc_volt_mag_diff('RCL11', rcl11_from_node_init_topo_noPV, rcl11_to_node_init_topo_noPV,
            rcl11_from_node_init_topo_PV, rcl11_to_node_init_topo_PV)

def calc_mid_topo():
    rcl11_from_node_init_topo_noPV = ['+6980.05-3.82254d', '+7148.58-121.614d', '+7014.24+117.724d']
    rcl11_to_node_init_topo_noPV = ['+7113.69-1.02614d', '+7002.28-122.147d', '+7143.01+118.103d']

    rcl11_from_node_init_topo_PV = ['+6980.05-3.82254d', '+7148.58-121.614d', '+7014.24+117.724d']
    rcl11_to_node_init_topo_PV = ['+7118.64-1.04383d', '+7007.42-122.164d', '+7147.9+118.088d']

    calc_volt_mag_diff('RCL9', rcl11_from_node_init_topo_noPV, rcl11_to_node_init_topo_noPV,
            rcl11_from_node_init_topo_PV, rcl11_to_node_init_topo_PV)

if __name__ == "__main__":
    # calc_init_topo()
    calc_mid_topo()
