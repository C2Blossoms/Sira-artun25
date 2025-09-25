# scripts/after_upload.py
# pyright: reportMissingImports=false, reportUndefinedVariable=false
from SCons.Script import Import  # มีให้ตอนที่ PlatformIO รันสคริปต์นี้
Import("env")

import os, sys, subprocess, time

def _after_upload(source, target, env):
    # หน่วงเวลาเล็กน้อยให้บอร์ดรีเซ็ต (ปรับได้จาก platformio.ini: custom_post_upload_delay)
    try:
        delay = float(env_.GetProjectOption("custom_post_upload_delay"))
    except Exception:
        delay = 2.8
    time.sleep(delay)

    # หา Python executable ที่ PlatformIO ใช้อยู่
    py = env.subst("$PYTHONEXE") or sys.executable or "python3"

    # ไฟล์ sender (promptpay_payload.py) ในโฟลเดอร์ src
    script = os.path.join(env.subst("$PROJECTSRC_DIR"), "promptpay_payload.py")

    print("[after_upload] running:", py, script)
    try:
        # -u = unbuffered เพื่อให้ log โชว์ทันทีใน Output ของ PIO
        subprocess.run([py, "-u", script], check=True)
    except Exception as e:
        print("[after_upload] ERROR:", e)

# hook หลัง upload สำเร็จ
env.AddPostAction("upload", _after_upload)
