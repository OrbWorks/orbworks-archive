// create the app
function FullPath(name) {
	var shell = new ActiveXObject("WScript.Shell");
	return shell.CurrentDirectory + "\\" + name;
}
	
var app = new ActiveXObject("CodeWarrior.CodeWarriorApp");
if (app) {
	var projname = FullPath("..\\OrbLaunch\\OrbLaunch.mcp");
	var project = app.OpenProject(projname, true, 0, 0);
	
	if (project) {
		var msgs = project.BuildAndWaitToCompleteWithOptions(0);
	} else {
		WScript.Echo("Unable to open OrbLaunch project");
	}
	
	projname = FullPath("..\\OrbFormsRT\\OrbFormsRT.mcp");
	project = app.OpenProject(projname, true, 0, 0);
	
	if (project) {
		project.SetCurrentTarget("OrbFormsRT (debug)");
		var msgs = project.BuildAndWaitToCompleteWithOptions(0);
		
		project.SetCurrentTarget("OrbFormsRT (release)");
		var msgs = project.BuildAndWaitToCompleteWithOptions(0);

		project.SetCurrentTarget("OrbFormsRT (FAT)");
		var msgs = project.BuildAndWaitToCompleteWithOptions(0);
	} else {
		WScript.Echo("Unable to open OrbFormsRT project");
	}

	app.Quit(0);
} else {
	WScript.Echo("Unable to create app object");
}
