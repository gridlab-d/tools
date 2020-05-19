# ***************************************
# Author: Jing Xie
# Created Date: 2020-5-1
# Updated Date: 2020-5-19
# Email: jing.xie@pnnl.gov
# ***************************************

import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

import re
import os.path


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

    def pre_process(self):
        """Get the Selected Data Columns
        """


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
    # ==Demo 01 (extract & plot Q-deltaV curves)
    p.read_csv()
    p.pre_process()


if __name__ == "__main__":
    test_CsvExt()
