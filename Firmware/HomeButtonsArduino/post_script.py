import os
import zipfile

Import("env")

partition_gen_path = "components/partition_table/gen_esp32part.py"

files_to_zip = ["firmware.bin", "bootloader.bin", "partitions.bin", "ota_data_initial.bin", "partitions.csv"]
zip_filename = "firmware.zip"


def create_zip(files, output_filename):
    with zipfile.ZipFile(output_filename, 'w') as zipf:
        for file in files:
            zipf.write(file, arcname=os.path.basename(file))
    print(f"{output_filename} created successfully.")


def create_partitions_csv():
    print("Creating partitions.csv...")
    package_dir = env.PioPlatform().get_package_dir("framework-espidf")
    path = os.path.join(package_dir, partition_gen_path)
    part_bin_path = os.path.join(env.subst("$BUILD_DIR"), "partitions.bin")
    part_csv_path = os.path.join(env.subst("$BUILD_DIR"), "partitions.csv")
    env.Execute(f"python {path} {part_bin_path} {part_csv_path}")
    print(f"{part_csv_path} created successfully.")


def post_build(source, target, env):
    print("#### POST BUILD ####")
    create_partitions_csv()
    files = [os.path.join(env.subst("$BUILD_DIR"), file) for file in files_to_zip]
    create_zip(files, os.path.join(env.subst("$BUILD_DIR"), zip_filename))

print("#### POST SCRIPT ####")
env.AddPostAction("buildprog", post_build)
