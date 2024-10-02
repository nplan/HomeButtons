#!/usr/bin/env python

import os

sdkconfig_files = ["sdkconfig.original_release", "sdkconfig.original_debug",
                   "sdkconfig.mini_release", "sdkconfig.mini_debug",
                   "sdkconfig.pro_release", "sdkconfig.pro_debug",
                   "sdkconfig.industrial_release", "sdkconfig.industrial_debug"]

def delete_sdkconfig_files():
    print("Deleting sdkconfig files...")

    for file in sdkconfig_files:
        if os.path.isfile(file):
            try:
                os.remove(file)
            except:
                print("Failed to delete {}".format(file))
            else:
                print("Deleted {}".format(file))


print("#### PRE SCRIPT ####")
delete_sdkconfig_files()
print("#### PRE SCRIPT DONE ####")
