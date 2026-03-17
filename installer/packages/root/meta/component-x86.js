function Component()
{
    component.setUpdateAvailable(false);

    if (systemInfo.kernelType === "darwin") {
        installer.setValue("TargetDir", installer.value("HomeDir") + "/");
    }
    // end with "/" in config.xml
    var targetDir = installer.value("TargetDir");
    var uninstaller = installer.value("MaintenanceToolName");

    if (systemInfo.kernelType === "linux") {
        installer.setValue("TargetDir", targetDir + "anylink");

        uninstaller = installer.value("TargetDir") + "/" + uninstaller;

    } else if (systemInfo.kernelType === "winnt") {
        installer.setValue("TargetDir", targetDir + "AnyLink");

        uninstaller = installer.value("TargetDir") + "/" + uninstaller + ".exe";
    } else if (systemInfo.kernelType === "darwin") {
        installer.setValue("TargetDir", targetDir + "AnyLink");

        uninstaller = installer.value("TargetDir") + "/" + uninstaller + ".app/Contents/MacOS/uninstall";
    }

    if (installer.isInstaller()) {
        if (installer.fileExists(uninstaller)) {
            installer.execute(uninstaller);
        }
//         console.log("anylink running status: " + installer.isProcessRunning("anylink"))
//         console.log("vpnui running status: " + installer.isProcessRunning("vpnui"))

        // request when component really installing
        component.addStopProcessForUpdateRequest("vpnui");
    }

    if (systemInfo.kernelType === "darwin") {
        component.addStopProcessForUpdateRequest("AnyLink");
    } else {
        // kill self when install and uninstall
        component.addStopProcessForUpdateRequest("anylink");
    }
}

Component.prototype.createOperations = function()
{
    if (systemInfo.kernelType === "linux") {
        installer.gainAdminRights()

        // default call createOperationsForArchive and then createOperationsForPath
        // The default implementation is recursively creating Copy and Mkdir operations for all files and folders within path.
        component.createOperations();

        // will be auto removed on uninstall
        component.addOperation("Copy", "@TargetDir@/anylink.desktop", "/usr/share/applications/anylink.desktop");

        // install and start the service or stop and remove the service
        component.addElevatedOperation("Execute", "@TargetDir@/bin/vpnagent","install",
                                "UNDOEXECUTE","@TargetDir@/bin/vpnagent","uninstall");
    } else if (systemInfo.kernelType === "winnt") {
        installer.gainAdminRights()

        component.createOperations();

        // This installer is built for x86 (32-bit). Always install the x86 vc_redist,
        // regardless of the CPU architecture reported by the host machine.
        component.addElevatedOperation("Execute", "{0,3010,1638,5100}", "@TargetDir@/vc_redist.x86.exe", "/norestart", "/q");

        //开始菜单快捷方式
        component.addOperation("CreateShortcut",
                               "@TargetDir@/anylink.exe",
                               "@StartMenuDir@/AnyLink Secure Client.lnk",
                               "workingDirectory=@TargetDir@");

        //桌面快捷方式
        component.addOperation("CreateShortcut",
                               "@TargetDir@/anylink.exe",
                               "@DesktopDir@/AnyLink Secure Client.lnk",
                               "workingDirectory=@TargetDir@");


        component.addElevatedOperation("Execute", "@TargetDir@/vpnagent.exe","install",
                                "UNDOEXECUTE","@TargetDir@/vpnagent.exe","uninstall");
    } else if (systemInfo.kernelType === "darwin") {
        component.createOperations();
        component.addOperation("CreateLink", "@ApplicationsDir@/AnyLink.app", "@TargetDir@/AnyLink.app");

        if (installer.gainAdminRights()) {
            // install and start the service or stop and remove the service
            component.addElevatedOperation("Execute", "@TargetDir@/AnyLink.app/Contents/MacOS/vpnagent","install",
                                    "UNDOEXECUTE","@TargetDir@/AnyLink.app/Contents/MacOS/vpnagent","uninstall");
            installer.dropAdminRights()
        }
    }
}
