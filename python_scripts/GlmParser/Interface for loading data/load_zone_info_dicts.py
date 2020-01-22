import pickle

def load_zone_info(zone_info_dicts_pickle_path_fn):
    hf_zone_info = open(zone_info_dicts_pickle_path_fn,'rb')
    new_node_zone_dict = pickle.load(hf_zone_info)
    new_load_zone_dict = pickle.load(hf_zone_info)
    hf_zone_info.close()
    
    return new_node_zone_dict, new_load_zone_dict
