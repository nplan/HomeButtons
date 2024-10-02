import subprocess
from tempfile import NamedTemporaryFile, TemporaryDirectory

from factory_data import FactoryData


def burn_efuses(port: str, baud: int, factory_data: FactoryData):
    factory_data = factory_data.encode()
    with NamedTemporaryFile("wb") as f:
        f.write(factory_data)
        f.flush()
        tokens = ["espefuse.py", "--port", port, "--baud", str(baud),
                  "--do-not-confirm",
                  "burn_block_data", "BLOCK3", f.name]
        try:
            subprocess.run(tokens, check=True, capture_output=True, text=True)
        except subprocess.CalledProcessError as e:
            print(e.stdout)
            quit()


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=str, required=True, help="Serial port")
    parser.add_argument("--baud", type=int, default=921600, help="Serial baud")

    parser.add_argument("--model_id", type=str, required=True,
                        help="Model ID of devices")
    parser.add_argument("--hw_version", type=str, required=True,
                        help="Hardware version of devices")
    parser.add_argument("--serial", type=str, required=True,
                        help="Serial number of device. If 'series', this is the first serial number.")
    parser.add_argument("--random_id", type=str, help="Random ID of device")
    args = parser.parse_args()

    if not args.random_id:
        args.random_id = FactoryData.generate_random_id()

    factory_data = FactoryData(args.model_id, args.hw_version, args.serial,
                               args.random_id)
    if not factory_data.verify():
        parser.error(f"Invalid factory data:\n {factory_data}")

    i = input(f"Burning:\n{factory_data}\nConfirm? [Y/n] ")
    if i.lower() != "y":
        print("Aborted.")
        quit()

    burn_efuses(args.port, args.baud, factory_data)
