import os
import zipfile
import csv


def increment_serial(serial: str):
    # example serial 2301-001
    serial = serial.split("-")
    serial[1] = str(int(serial[1]) + 1).zfill(3)
    return "-".join(serial)


def find_espidf():
    packages = os.path.join(os.path.expanduser("~"), ".platformio", "packages")
    assert os.path.exists(packages)
    for package in os.listdir(packages):
        if package == "framework-espidf":
            return os.path.join(packages, package)
    raise Exception("Could not find esp-idf platformio package")


def unzip_file(zip_path: str, extract_dir: str):
    with zipfile.ZipFile(zip_path, 'r') as zip_ref:
        zip_ref.extractall(extract_dir)


def load_partition_table(csv_path: str):
    data_dict = {}
    with open(csv_path, 'r') as file:
        csv_reader = csv.reader(file)
        next(csv_reader)
        next(csv_reader)
        for row in csv_reader:
            name = row[0].strip()
            data_type = row[1].strip()
            sub_type = row[2].strip()
            offset = row[3].strip()
            size = row[4].strip()
            if "K" in size:
                size = hex(int(size.replace("K", "")) * 1024)
            elif "M" in size:
                size = hex(int(size.replace("M", "")) * 1024 * 1024)
            flags = row[5].strip()
            data_dict[name] = {
                'Type': data_type,
                'SubType': sub_type,
                'Offset': offset,
                'Size': size,
                'Flags': flags
            }
    return data_dict
