#!/usr/bin/env node

var fs = require('fs'),
	path = require('path'),
	uuid = require('node-uuid');

require("../magpie/platforms/android/cordova/node_modules/shelljs/global")

echo( "Magpie, Version: 1.0.20140819" );
echo( "Based on: Cordova v3.5.0");
echo( "Tested:  Cocos2d-X v2.2.1\n");

if( process.argv.length < 3 ) {
	echo("Missing arguments, syntax:\nmagpie.js <cocos2dx_proj_dir>\nAbort.\n");
	exit(1);
}

var magpie_cli = process.argv[1];
var proj_dir = process.argv[2];

var magpie_dir = path.resolve( path.dirname(magpie_cli) + "/../magpie" );
var proj_dir = path.resolve( proj_dir );
echo("--- running with arguments:\nmagpie = " + magpie_dir + "\nproj_dir = " + proj_dir + "\n");

echo("--- checking project files ... ");
var android_manifest = proj_dir + "/proj.android/AndroidManifest.xml";
var android_mk_src = proj_dir + "/proj.android/jni/Android.mk";
var package_id = "";
var app_name = "";

if( test('-f', android_manifest) && test('-f', android_mk_src) ) {
	echo( "AndroidManifest.xml found: " + android_manifest );
	echo( "Android.mk found: " + android_mk_src );
	package_id = grep("package=", android_manifest).replace(/package=/g, '').replace(/[ "\t\n]*/g,'');
	echo( "package id: " + package_id );
	
	app_name = grep("<activity android:name=\".", android_manifest).replace(/activity android:name=/g, '').replace(/[ ".<\t\n]*/g,'');
	echo( "app name: " + app_name );
} else {
	echo("Error, not a valid cocos2d-x project.\nAbort.\n");
	exit(1);
}

if (!String.prototype.startsWith) {
	  Object.defineProperty(String.prototype, 'startsWith', {
	    enumerable: false,
	    configurable: false,
	    writable: false,
	    value: function (searchString, position) {
	      position = position || 0;
	      return this.lastIndexOf(searchString, position) === position;
	    }
	  });
	}
if (!String.prototype.endsWith) {
	  Object.defineProperty(String.prototype, 'endsWith', {
	    value: function (searchString, position) {
	      var subjectString = this.toString();
	      if (position === undefined || position > subjectString.length) {
	        position = subjectString.length;
	      }
	      position -= searchString.length;
	      var lastIndex = subjectString.indexOf(searchString, position);
	      return lastIndex !== -1 && lastIndex === position;
	    }
	  });
	}

var main_java = app_name + ".java";
var main_files = find(proj_dir + "/proj.android/src").filter(function(file) { return file.endsWith(main_java); });

var main_java_src = "";
if( main_files.length > 0 ) {
	main_java_src = main_files[0];
	echo( "main class: " + main_java_src );
} else {
	echo( "Error, file not found: " + main_java + "\nAbort.\n" );
	exit(1);
}

var proj_root = path.resolve( proj_dir + "/../../proj_" + app_name)
if( test('-d', proj_root) ) {
	echo( "Error, target folder already exists: " + proj_root + "\nAbort.\n" );
	exit(0);
}

echo( "new project folder: " + proj_root );
echo( "\n--- processing project files ..." )

var proj_newdir = proj_root + "/platforms";
mkdir('-p', proj_newdir);
cp( "-R", proj_dir + "/*", proj_newdir + "/" );
fs.renameSync( proj_newdir + "/proj.android", proj_newdir + "/android" );
fs.renameSync( proj_newdir + "/proj.ios", proj_newdir + "/ios" );
var droid_newdir = proj_newdir + "/android";
var ios_newdir = proj_newdir + "/ios";
echo( "copy from cocos2d-x project files ... ok");

cp( "-R", magpie_dir + "/hooks", proj_root + "/");
cp( "-R", magpie_dir + "/merges", proj_root + "/");
cp( "-R", magpie_dir + "/plugins", proj_root + "/");
cp( "-R", magpie_dir + "/www", proj_root + "/");
echo( "copying magpie project files ... ok" );

cp( magpie_dir + "/platforms/common/Magpie.*", proj_newdir + "/Classes/");
echo( "copying common C++ code ... ok");

var app_delegate_cpp = proj_newdir + "/Classes/AppDelegate.cpp";
if( test('-f', app_delegate_cpp) ) {
	fs.appendFileSync( app_delegate_cpp, "\n\n#include \"Magpie.h\"\n\n// shared Magpie sington\nstatic Magpie s_sharedMagpie;\n\n" );
	echo( "patching AppDelegate.cpp ... ok");
} else {
	echo( "Error, file not found: " + app_delegate_cpp + "\nAbort.\n" );
	exit(0);
}

if( ! test('-d', droid_newdir + "/jni") ) {
	mkdir('-p', droid_newdir + "/jni" );
}
if( ! test('-d', droid_newdir + "/libs") ) {
	mkdir('-p', droid_newdir + "/libs" );
}
if( ! test('-d', droid_newdir + "/res/xml") ) {
	mkdir('-p', droid_newdir + "/res/xml" );
}
cp( magpie_dir + "/config.xml", proj_root + "/" );
cp( '-R', magpie_dir + "/platforms/android/cordova", droid_newdir + "/" );
cp( '-R', magpie_dir + "/platforms/android/jni/MagpieBridgeJni.*", droid_newdir + "/jni/" );
cp( '-R', magpie_dir + "/platforms/android/libs/magpie-framework.jar", droid_newdir + "/libs/" );
cp( '-R', magpie_dir + "/platforms/android/res/xml/config.xml", droid_newdir + "/res/xml/" );
echo( "copying magpie android files ... ok");

var main_files = find(droid_newdir + "/src").filter(function(file) { return file.endsWith(main_java); });
if( main_files.length > 0 ) {
	main_java_dest = main_files[0];
}

fs.writeFileSync( main_java_dest, 
		fs.readFileSync( main_java_src, 'utf8' )
		.replace("com.handywit.__AppName__", package_id)
		.replace("__AppName__", app_name) 
		, "utf8" );
echo( "replacing main activity java: " + main_java + " ... ok" );

fs.writeFileSync( droid_newdir + "/jni/Android.mk", 
		fs.readFileSync( proj_dir + "/proj.android/jni/Android.mk", 'utf8' )
		.replace("hellocpp/main.cpp", "hellocpp/main.cpp \\\n                   MagpieBridgeJni.cpp")
		, "utf8" );
echo( "patching Android.mk ... ok" );

cp( '-R', magpie_dir + "/platforms/ios/cordova", ios_newdir + "/" );
cp( '-R', magpie_dir + "/platforms/ios/CordovaLib", ios_newdir + "/" );
cp( '-R', magpie_dir + "/platforms/ios/Magpie.Framework", ios_newdir + "/" );
cp( '-f', magpie_dir + "/platforms/ios/MagpieBridgeiOS.*", ios_newdir + "/" );
cp( '-f', magpie_dir + "/platforms/ios/AppController.*", ios_newdir + "/" );
cp( '-R', magpie_dir + "/platforms/ios/__AppName__", ios_newdir + "/" );
fs.renameSync( ios_newdir + "/__AppName__", ios_newdir + "/" + app_name );
echo( "copying magpie ios files ... ok" );

var configs = [ proj_root + "/config.xml", 
                droid_newdir + "/res/xml/config.xml",
                ios_newdir + "/" + app_name + "/config.xml"
               ];
for(var i in configs) {
	var config_content = fs.readFileSync( configs[i], 'utf8' ).replace("com.handywit.__AppName__", package_id).replace("__AppName__", app_name);
	fs.writeFileSync( configs[i], config_content, "utf8" );
}
echo( "patching config.xml ... ok" );

echo( "\n--- patching xcode project file ..." );
var xcode_proj_file = ios_newdir + "/" + app_name + ".xcodeproj/project.pbxproj";
echo( "target file: " + xcode_proj_file );

var xcode_proj_content = fs.readFileSync( xcode_proj_file, 'utf8' );

var info_plist_newpath = app_name + "/" + app_name + "-Info.plist";
mv( ios_newdir + "/Info.plist", ios_newdir + "/" + info_plist_newpath);
xcode_proj_content = xcode_proj_content
	.replace(/ Info.plist/g, " " + info_plist_newpath)
	.replace(/\.\.\/proj\.ios/g, "../ios");
echo( "moving and renaming Info.plist ... ok");

// insert group "Plugins"
var classes_tag = grep(/\/\* Classes \*\/,/, xcode_proj_file).replace("Classes","").replace(/[ \t\r\n\*\/,]*/g, "");
var insert_plugins_entry = "307C750510C5A3420062BCA9 /* Plugins */,\n                                ";
var insert_plugins_group = "307C750510C5A3420062BCA9 /* Plugins */ = {\n\
                        isa = PBXGroup;\n\
                        children = (\n\
                        );\n\
                        name = Plugins;\n\
                        path = " + app_name + "/Plugins;\n\
                        sourceTree = SOURCE_ROOT;\n\
                };\n                ";
var insert_before_entry = classes_tag + " /* Classes */,";
var insert_before_group = classes_tag + " /* Classes */ = {"
xcode_proj_content = xcode_proj_content.replace(insert_before_entry, insert_plugins_entry + insert_before_entry)
	.replace(insert_before_group, insert_plugins_group + insert_before_group);
echo("adding entry 'Plugins' ... ok");

var allUuids = [];
function generateUuid() {
    var id = uuid.v4()
                .replace(/-/g,'')
                .substr(0,24)
                .toUpperCase();

    if (allUuids.indexOf(id) >= 0) {
        return this.generateUuid();
        
    } else if (xcode_proj_content.indexOf(id) >= 0) {
    	allUuids.push( id );
        return this.generateUuid();
        
    } else {
    	allUuids.push( id );
        return id;
    }
}

var fileTypes = {
	".h": "sourcecode.c.h",
	".cpp": "sourcecode.cpp.cpp",
	".m": "sourcecode.c.objc",
	".mm": "sourcecode.cpp.objcpp",
	".framework" : "wrapper.framework"
};

function insertFile( filepaths, group, beforePath ) {
	var beforeFile = path.basename( beforePath );
	var beforeFileType = fileTypes[ path.extname( beforeFile ) ];
	var beforeFileInGroup = beforeFile + " in " + group;
	var patterns = [
	              "\\/\\* " + beforeFile + " \\*\\/,",
	              "\\/\\* " + beforeFileInGroup + " \\*\\/,",
	              "\\/\\* " + beforeFile + " \\*\\/ = {",
	              "\\/\\* " + beforeFileInGroup + " \\*\\/ = {"
	              ]
	var lines = [];
	for(var i=0; i<patterns.length; i++) {
		var re = new RegExp(patterns[i]);
		var line = grep( re, xcode_proj_file );
		lines.push( line );
	}
	var beforeFileTag = lines[0].replace(beforeFile,"").replace(/[ \t\r\n\*\/,]*/g, "");
	var beforeGroupTag = lines[1].replace(beforeFileInGroup,"").replace(/[ \t\r\n\*\/,]*/g, "");
	if(beforeFileTag.length == 0 || beforeGroupTag.length == 0) {
		echo( "Error: entry not found: " + beforePath );
		exit(1);
		return false;
	}
	
	var insertLines = ["", "", "", ""];
	for(var i in filepaths) {
		var addPath = filepaths[i];
		var addFile = path.basename( addPath );
		var addFileType = fileTypes[ path.extname(addFile) ];
		var addFileInGroup = addFile + " in " + group;
		var addFileTag = generateUuid();
		var addGroupTag = generateUuid();
		
		for(var j in lines) {
			var addLine = lines[ j ]
				.replace(beforeFileTag, addFileTag)
				.replace(beforeGroupTag, addGroupTag)
				.replace(beforeFileInGroup, addFileInGroup)
				.replace(beforePath, addPath)
				.replace(beforeFileType, addFileType)
				.replace(beforeFile, addFile)
				.replace(beforeFile, addFile);
			insertLines[j] += addLine;
		}
		
		echo( "adding entry '" + addPath + "' to '" + group + "' ... ok");
	}
	
	for(var j in lines) {
		xcode_proj_content = xcode_proj_content.replace(lines[j], insertLines[j] + lines[j]);
	}
	
	return true;
}

insertFile( ["Magpie.framework"], "Frameworks", "System/Library/Frameworks/Foundation.framework" );
insertFile( ["MagpieBridgeiOS.h", "MagpieBridgeiOS.mm"], "Sources", "main.m" );
insertFile( ["../Classes/Magpie.h", "../Classes/Magpie.cpp"], "Sources", "AppDelegate.cpp" );

fs.writeFileSync( xcode_proj_file, xcode_proj_content, "utf8" );
echo( "writting xcode project files ... ok");

echo( "done.\n" );

echo( "project folder: " + proj_root )

echo( "\nGood luck!\n")



