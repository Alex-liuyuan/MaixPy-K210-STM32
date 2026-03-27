from sysu import app, model

print("[app main] hello from bundled SYSU_AIOTOS app")
print("[app main] app id =", app.app_id())
print("[app main] script =", app.script_path())
print("[app main] storage =", app.storage_backend())
print("[app main] model path =", app.model_path())
print("[app main] model info =", model.info())
print("[app main] labels =", model.labels_path())

with open("/models/README.txt", "r") as f:
    print("[app main] model slot =", f.readline().strip())

try:
    from sysu import nn

    runner = nn.NN()
    print("[app main] nn info =", runner.info())
    runner.close()
except Exception as e:
    print("[app main] nn unavailable =", e)
