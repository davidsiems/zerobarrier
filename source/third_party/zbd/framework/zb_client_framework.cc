
#include "zb_client_framework.h"

#include "../zbd.c"
#include "../controls/zb_controls.cc"
#include "../graphics/zb_graphics.cc"
#include "../io/zb_io.cc"
#include "../math/zb_math.cc"
#include "../script/zb_script.cc"
#include "../time/zb_time.cc"

#include "internal/entry_win32.cc"
#include "internal/os_win32.cc"

#include "../controls/controls_v8_bindings.cc"
#include "../graphics/graphics_v8_bindings.cc"
#include "internal/os_win32_v8_bindings.cc"

void ReportVMError(const VMError &errors) {
  zb_spam("Script Error: %s(%d) -- %s\n%s", errors.file.c_str(), errors.line, errors.message.c_str(), errors.callstack.c_str());
}

void Launch(OS *os) {
  Controls controls;
  os->controls = &controls;

  ViewParameters vp;
  vp.width = 1280;
  vp.height = 720;
  vp.windowed = true;

  os->Boot();
  os->InitializeView(vp.width, vp.height);

  Graphics graphics;
  vp.view = os->GetWindowHandle();
  graphics.Initialize(vp);

  VM vm;
  VMError errors;

  do {
    errors.Reset();
    
    vm.Initialize();
    vm.EnableDebugging("client", 5858);
    SandboxVM(&vm);
    AddVMBindings(&vm);
    AddJSONBindings(&vm);
    AddOSBindings(&vm, os);
    AddControlsBindings(&vm, &controls);
    AddGraphicsBindings(&vm, &graphics);

    VMRequireFile("js/require.js", &vm, &errors);
    VMRequireFile("js/boot.js", &vm, &errors);

    if (!errors.HasError()) {
      vm.Call("main", &errors);
    }
    
    if (errors.HasError()) {
      ReportVMError(errors);

      if (errors.isRecoverable) {
        errors.Reset();
        vm.Call("onError", &errors);
      }
    }

    vm.DisableDebugging();
  } while (vm.ReturnValueToString() == "false");

  os->Shutdown();
}
