"""enlg Cloud Handler Module.

Handler signature: handler(target, op_args: list, env=None) -> result
"""


def handle_cloud_deploy(target, op_args: list, env=None):
    """deploy service using config"""
    config = op_args[0] if op_args else {}
    print(f"[enlg Cloud] Deploying service '{target}' with config {config}...")
    return {"service": str(target), "status": "DEPLOYED", "config": config}


def handle_cloud_fetch(target, op_args: list, env=None):
    """cloud_fetch data from bucket"""
    source = op_args[0] if op_args else "unknown"
    print(f"[enlg Cloud] Fetching '{target}' from '{source}'...")
    return {"data": str(target), "source": str(source), "status": "OK"}


CLOUD_HANDLERS: dict = {
    "CLOUD_DEPLOY": handle_cloud_deploy,
    "CLOUD_FETCH":  handle_cloud_fetch,
}
