Import("env")

import shutil

#print("Current CLI targets", COMMAND_LINE_TARGETS)
#print("Current Build targets", BUILD_TARGETS)


def process_firmware(source, target, env):
    print("copy FIRMWARE to /bin folder.")
    program_path = target[0].get_abspath()
    print("FIRMWARE path", program_path)
    # Use case: sign a firmware, do any manipulations with ELF, etc
    # env.Execute(f"sign --elf {program_path}")
    shutil.copy2(program_path, "bin") 





#env.AddPostAction("$PROGPATH", post_program_action)

#
# Upload actions
#

def before_upload(source, target, env):
    print("before_upload")
    # do some actions

    # call Node.JS or other script
    env.Execute("node --version")


def after_upload(source, target, env):
    print("after_upload")
    # do some actions

#env.AddPreAction("upload", before_upload)
#env.AddPostAction("upload", after_upload)

env.AddPostAction("$BUILD_DIR/firmware.bin", process_firmware)