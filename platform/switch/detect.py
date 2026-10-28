import os
import sys
from typing import TYPE_CHECKING

from methods import print_error, print_warning

if TYPE_CHECKING:
    from SCons.Script.SConscript import SConsEnvironment


def get_name():
    return "Switch"


def can_build():
    dkp = os.environ.get("DEVKITPRO")
    if not dkp:
        return False
    return os.path.exists(os.path.join(dkp, "devkitA64")) and os.path.exists(os.path.join(dkp, "libnx"))


def get_opts():
    from SCons.Variables import BoolVariable

    return [
        BoolVariable("touch", "Enable touch events", True),
    ]


def get_doc_classes():
    return []


def get_doc_path():
    return "doc_classes"


def get_flags():
    return {
        "arch": "arm64",
        "target": "template_release",
        "supported": [],
        # Keeps --scene / --main-pack usable from <app dir>/args.txt.
        "disable_path_overrides": False,
        # Switch GPU is reached through Mesa's nouveau EGL; no Vulkan, no RenderingDevice.
        "vulkan": False,
        "opengl3": True,
        "d3d12": False,
        "metal": False,
        "sdl": False,
        "accesskit": False,
        "angle": False,
        "use_volk": False,
        "builtin_pcre2_with_jit": False,
        # Heavy or x86/desktop-only modules.
        "module_text_server_adv_enabled": False,
        "module_text_server_fb_enabled": True,
        "module_raycast_enabled": False,
        "module_jolt_physics_enabled": False,
        "module_lightmapper_rd_enabled": False,
        "module_glslang_enabled": False,
        "module_openxr_enabled": False,
        "module_webxr_enabled": False,
        "module_mobile_vr_enabled": False,
        "module_upnp_enabled": False,
        "module_camera_enabled": False,
        "module_mono_enabled": False,
        "module_webrtc_enabled": False,
        "module_websocket_enabled": False,
        "module_enet_enabled": False,
        "module_mbedtls_enabled": False,
        "module_multiplayer_enabled": False,
        "module_basis_universal_enabled": False,
        "module_astcenc_enabled": False,
        "module_betsy_enabled": False,
        "module_fbx_enabled": False,
        "module_gltf_enabled": False,
        "module_noise_enabled": True,
        "module_minimp3_enabled": True,
        "module_vorbis_enabled": True,
    }


def configure(env: "SConsEnvironment"):
    dkp = os.environ["DEVKITPRO"]
    bin_dir = f"{dkp}/devkitA64/bin"

    env["CC"] = f"{bin_dir}/aarch64-none-elf-gcc"
    env["CXX"] = f"{bin_dir}/aarch64-none-elf-g++"
    env["LINK"] = env["CXX"]
    env["AR"] = f"{bin_dir}/aarch64-none-elf-gcc-ar"
    env["RANLIB"] = f"{bin_dir}/aarch64-none-elf-gcc-ranlib"
    env["ENV"]["DEVKITPRO"] = dkp
    env.PrependENVPath("PATH", f"{dkp}/portlibs/switch/bin")
    env.PrependENVPath("PATH", bin_dir)

    arch = ["-march=armv8-a+crc+crypto", "-mtune=cortex-a57", "-mtp=soft", "-fPIE"]
    env.Prepend(CCFLAGS=arch + ["-ffunction-sections", "-fdata-sections", "-ffp-contract=off", "-pipe"])
    env.Prepend(CPPPATH=[f"{dkp}/portlibs/switch/include", f"{dkp}/libnx/include"])
    env.Prepend(LIBPATH=[f"{dkp}/portlibs/switch/lib", f"{dkp}/libnx/lib"])
    env.Prepend(LINKFLAGS=arch + [f"-specs={dkp}/libnx/switch.specs", "-Wl,--gc-sections"])

    env.Prepend(CXXFLAGS=["-include", "platform/switch/compat/switch_compat.h"])
    env.Append(CPPDEFINES=["__SWITCH__", "HORIZON_ENABLED", "UNIX_ENABLED", ("_FILE_OFFSET_BITS", 64)])

    if env["touch"]:
        env.Append(CPPDEFINES=["TOUCH_ENABLED"])

    if env["opengl3"]:
        env.Append(CPPDEFINES=["GLES3_ENABLED"])

    # Order matters for static libs: Mesa EGL/GL stack first, then libnx.
    env.Append(LIBS=["EGL", "GLESv2", "glapi", "drm_nouveau", "nx", "m"])

    env.Prepend(CPPPATH=["#platform/switch", "#platform/switch/compat"])
