from load_zone_info_dicts import load_zone_info

if __name__ == '__main__':
    zone_info_dicts_pickle_path_fn = 'zone_info'
    
    new_node_zone_dict, new_load_zone_dict = load_zone_info(zone_info_dicts_pickle_path_fn)

    print(len(new_node_zone_dict))
    print(len(new_load_zone_dict))

    #print(new_node_zone_dict.keys())

    print(new_node_zone_dict['n256851437_1207'])
    print(new_node_zone_dict['n256860358_1207'])    

    print(new_load_zone_dict['39693222_1207'])
    print(new_load_zone_dict['39695307_1207'])
