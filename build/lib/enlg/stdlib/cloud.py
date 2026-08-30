
import enlg.runtime.domain_handlers.cloud_handlers as _cloud
def deploy(target, *args): return _cloud.handle_cloud_deploy(target, list(args))
def upload(target, *args): return _cloud.handle_cloud_upload(target, list(args))
