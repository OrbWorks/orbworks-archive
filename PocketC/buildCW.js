// create the app
function FullPath(name) {
	var shell = new ActiveXObject("WScript.Shell");
	return shell.CurrentDirectory + "\\" + name;
}
	
var app = new ActiveXObject("CodeWarrior.CodeWarriorApp");
if (app) {
	var projname = FullPath("PocketC-Palm\\PocketC.mcp");
	var project = app.OpenProject(projname, true, 0, 0);
	
	if (project) {
		project.SetCurrentTarget("PocketC (shareware)");
		var msgs = project.BuildAndWaitToCompleteWithOptions(0);
		
		project.SetCurrentTarget("PocketC (retail)");
		var msgs = project.BuildAndWaitToCompleteWithOptions(0);
		
		project.SetCurrentTarget("PocketC Runtime");
		var msgs = project.BuildAndWaitToCompleteWithOptions(0);
		
		project.SetCurrentTarget("PocketC Scanner Runtime");
		var msgs = project.BuildAndWaitToCompleteWithOptions(0);
		
		project.SetCurrentTarget("PocketC FAT");
		var msgs = project.BuildAndWaitToCompleteWithOptions(0);
	} else {
		WScript.Echo("Unable to open PocketC project");
	}

	app.Quit(0);
} else {
	WScript.Echo("Unable to create app object");
}
