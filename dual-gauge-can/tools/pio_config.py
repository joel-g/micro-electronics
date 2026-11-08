"""Run before compilation, not as a late buildprog action: headers must exist first."""
Import("env")
import importlib.util
from pathlib import Path

root=Path(env.subst("$PROJECT_DIR"))
spec=importlib.util.spec_from_file_location("instrument_configure",root/"tools/configure.py")
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
source=Path(env.GetProjectOption("custom_gauge_config","config/instrument.json"))
if not source.is_absolute():source=root/source
out=Path(env.subst("$BUILD_DIR"))/"generated"
try:
    c=module.generate(source,out/"instrument_config.h")
except module.ConfigError as e:
    print(f"Configuration error: {e}")
    env.Exit(1)
env.Prepend(CPPPATH=[str(out)])
print(f'Instrument config: {source.name}, CAN {c["can"]["mode"]} @ {c["can"]["bitrate"]}, ID {c["can"]["id"]}')
