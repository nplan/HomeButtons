#!/usr/bin/env python

import os


def delete_sdkconfig_files():
    try:
        os.remove("sdkconfig.release")
        os.remove("sdkconfig.debug")
        print("Deleted sdkconfig.release and sdkconfig.debug files")
    except:
        pass


delete_sdkconfig_files()
