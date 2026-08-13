"""enlg Domain Handler Registry.

Central registry that maps domain operation strings to handler functions.
To add a new operation: define a handler function in the appropriate
module (ml_handlers.py, dl_handlers.py, etc.) and add it to the dict.

vm.py dispatches here — never needs to change again.
"""

from enlg.runtime.domain_handlers.ml_handlers import ML_HANDLERS
from enlg.runtime.domain_handlers.dl_handlers import DL_HANDLERS
from enlg.runtime.domain_handlers.sec_handlers import SEC_HANDLERS
from enlg.runtime.domain_handlers.cloud_handlers import CLOUD_HANDLERS

HANDLER_REGISTRY: dict = {}
HANDLER_REGISTRY.update(ML_HANDLERS)
HANDLER_REGISTRY.update(DL_HANDLERS)
HANDLER_REGISTRY.update(SEC_HANDLERS)
HANDLER_REGISTRY.update(CLOUD_HANDLERS)
