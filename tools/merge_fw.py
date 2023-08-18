import os
import subprocess
import argparse
import zipfile
from tempfile import TemporaryDirectory
from helpers import *

fw_zip_contents = ["firmware.bin", "bootloader.bin",
                   "partitions.bin", "ota_data_initial.bin", "partitions.csv"]


def merge_fw(fw_zip_path: str, out_path: str, spiffs_img_path: str = None):
    with TemporaryDirectory() as tmp_dir:
        # unzip firmware
        unzip_file(fw_zip_path, tmp_dir)

        # check if all files are present
        missing = []
        for file in fw_zip_contents:
            if not os.path.isfile(os.path.join(tmp_dir, file)):
                missing.append(file)
        if len(missing) > 0:
            print("Missing files in firmware zip: " + str(missing))
            quit()

        partitions = load_partition_table(
            os.path.join(tmp_dir, "partitions.csv"))

        tokens = ["esptool.py", "--chip", "ESP32-S2", "merge_bin", "-o", out_path,
                  "0x1000", os.path.join(tmp_dir, "bootloader.bin"),
                  "0x8000", os.path.join(tmp_dir, "partitions.bin"),
                  partitions["otadata"]["Offset"], os.path.join(
                      tmp_dir, "ota_data_initial.bin"),
                  partitions["app0"]["Offset"], os.path.join(tmp_dir, "firmware.bin")]

        if spiffs_img_path is not None:
            tokens += [partitions["spiffs"]["Offset"], spiffs_img_path]

        try:
            subprocess.run(tokens, check=True, capture_output=True, text=True)
        except subprocess.CalledProcessError as e:
            print(e.stdout)
            quit()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='Merge firmware files into one binary.')
    parser.add_argument('fw_zip', type=str,
                        help='Path to firmware zip file.')
    parser.add_argument('out', type=str,
                        help='Path to output binary.')
    parser.add_argument('--spiffs', type=str, default=None,
                        help='Path to spiffs image.')
    args = parser.parse_args()

    merge_fw(args.fw_zip, args.out, args.spiffs)
