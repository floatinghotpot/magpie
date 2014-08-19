#!/usr/bin/env node

var fs = require('fs'),
	path = require('path'),
	uuid = require('node-uuid');

require("../magpie/platforms/android/cordova/node_modules/shelljs/global")

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

var magpie_cli = {};

magpie_cli = {
	main: function( argv ) {
		echo( "Magpie, Version: 1.0.20140819" );
		echo( "Built on: Cordova v3.5.0");
		echo( "Tested:  Cocos2d-X v2.2.1\n");

		if( argv.length >= 3 ) {
			this.magpie_dir = path.resolve( path.dirname(argv[1]) + "/../magpie" );
			this.proj_dir = path.resolve( argv[2] );
			echo("--- running with arguments:\nmagpie = " + this.magpie_dir + "\nproj_dir = " + this.proj_dir + "\n");
			
			if( this.check_project() ) {
				if( this.get_target_dir() && this.import_project() ) {
					echo( "done.\n" );
					echo( "project new root folder: " + this.proj_root )
					echo( "\nGood luck!\n")
					return true;
				}
			} else {
				echo("Not a valid cocos2d-x project.");
			}
		} else {
			echo("Missing arguments, syntax:\nmagpie.js <cocos2dx_proj_dir>\nAbort.\n");
		}
		
		echo( "\nAbort.\n" );
		return false;
	},
	check_project: function() {
		echo("--- checking project files ... ");
		
		return this.check_manifest() && 
			this.check_xcode_project() &&
			this.check_android_mk() && 
			this.check_main_java();
	},
	check_manifest: function() {
		this.android_manifest = this.proj_dir + "/proj.android/AndroidManifest.xml";
		this.package_id = "";
		this.app_name = "";

		if( test('-f', this.android_manifest) ) {
			echo( "AndroidManifest.xml: " + this.android_manifest );
			this.package_id = grep("package=", this.android_manifest).replace(/package=/g, '').replace(/[ "\t\n]*/g,'');
			echo( "package id: " + this.package_id );
			
			this.app_name = grep("<activity android:name=\".", this.android_manifest).replace(/activity android:name=/g, '').replace(/[ ".<\t\n]*/g,'');
			echo( "app name: " + this.app_name );
			
			return true;
		}
		
		echo( "AndroidManifest.xml not found" );
		return false;
	},
	check_android_mk: function() {
		this.android_mk_src = this.proj_dir + "/proj.android/jni/Android.mk";
		if ( test('-f', this.android_mk_src) ) {
			echo( "Android.mk: " + this.android_mk_src );
			return true;
		}
		
		echo( "Android.mk not found" );
		return false;
	},
	check_main_java: function() {
		var main_java = this.app_name + ".java";
		var files = find(this.proj_dir + "/proj.android/src").filter(function(file) { return file.endsWith(main_java); });

		if( files.length > 0 ) {
			this.main_java = main_java;
			echo( "main class: " + files[0] );
			return true;
		}

		echo( "Error, file not found: " + this.main_java );
		return false;
	},
	check_xcode_project: function() {
		this.xcode_proj_src = this.proj_dir + "/proj.ios/" + this.app_name + ".xcodeproj/project.pbxproj";
		if ( test('-f', this.xcode_proj_src) ) {
			echo( "Xcode project file: " + this.xcode_proj_src );
			return true;
		}
		
		echo( "Xcode project file not found" );
		return false;
	},
	get_target_dir: function() {
		this.proj_root = path.resolve( this.proj_dir + "/../../proj_" + this.app_name)
		
		if( test('-d', this.proj_root) ) {
			echo( "Error, target folder already exists: " + this.proj_root );
			return false;
		}

		echo( "\ntarget project root: " + this.proj_root );
		mkdir('-p', this.proj_root);
		
		this.proj_newdir = this.proj_root + "/platforms";
		mkdir('-p', this.proj_newdir);

		return true;
	},
	import_project: function() {
		echo( "\n--- importing project files ...");
		
		return this.copy_cocos2d_files() && 
			this.copy_magpie_files() && 
			this.patch_files();
	},
	copy_cocos2d_files: function() {
		cp( "-R", this.proj_dir + "/*", this.proj_newdir + "/" );
		fs.renameSync( this.proj_newdir + "/proj.android", this.proj_newdir + "/android" );
		fs.renameSync( this.proj_newdir + "/proj.ios", this.proj_newdir + "/ios" );
		this.droid_newdir = this.proj_newdir + "/android";
		this.ios_newdir = this.proj_newdir + "/ios";
		echo( "copy from cocos2d-x project files ... ok");
		
		return true;
	},
	copy_magpie_files: function() {
		cp( "-R", this.magpie_dir + "/hooks", this.proj_root + "/");
		cp( "-R", this.magpie_dir + "/merges", this.proj_root + "/");
		cp( "-R", this.magpie_dir + "/plugins", this.proj_root + "/");
		cp( "-R", this.magpie_dir + "/www", this.proj_root + "/");
		echo( "copying magpie project files ... ok" );
		
		cp( this.magpie_dir + "/platforms/common/Magpie.*", this.proj_newdir + "/Classes/");
		echo( "copying common C++ code ... ok");
		
		if( ! test('-d', this.droid_newdir + "/jni") ) {
			mkdir('-p', this.droid_newdir + "/jni" );
		}
		if( ! test('-d', this.droid_newdir + "/libs") ) {
			mkdir('-p', this.droid_newdir + "/libs" );
		}
		if( ! test('-d', this.droid_newdir + "/res/xml") ) {
			mkdir('-p', this.droid_newdir + "/res/xml" );
		}

		cp( this.magpie_dir + "/config.xml", this.proj_root + "/" );
		
		var droid_srcdir =  this.magpie_dir + "/platforms/android";
		cp( '-R', droid_srcdir + "/cordova", this.droid_newdir + "/" );
		cp( '-R', droid_srcdir + "/jni/MagpieBridgeJni.*", this.droid_newdir + "/jni/" );
		cp( '-R', droid_srcdir + "/libs/magpie-framework.jar", this.droid_newdir + "/libs/" );
		cp( '-R', droid_srcdir + "/res/xml/config.xml", this.droid_newdir + "/res/xml/" );
		echo( "copying magpie android files ... ok");
		
		var ios_srcdir = this.magpie_dir + "/platforms/ios";
		cp( '-R', ios_srcdir + "/cordova", this.ios_newdir + "/" );
		cp( '-R', ios_srcdir + "/CordovaLib", this.ios_newdir + "/" );
		cp( '-R', ios_srcdir + "/Magpie.framework", this.ios_newdir + "/" );
		cp( '-f', ios_srcdir + "/MagpieBridgeiOS.*", this.ios_newdir + "/" );
		cp( '-f', ios_srcdir + "/AppController.*", this.ios_newdir + "/" );
		cp( '-R', ios_srcdir + "/__AppName__", this.ios_newdir + "/" );
		fs.renameSync( this.ios_newdir + "/__AppName__", this.ios_newdir + "/" + this.app_name );
		echo( "copying magpie ios files ... ok" );

		return true;
	},
	patch_files: function() {
		return this.patch_app_delegate_cpp() &&
			this.replace_main_java() &&
			this.update_config_xml() &&
			this.patch_android_mk() &&
			this.patch_xcode_project_file();
	},
	patch_app_delegate_cpp: function() {
		var app_delegate_cpp = this.proj_newdir + "/Classes/AppDelegate.cpp";
		if( test('-f', app_delegate_cpp) ) {
			fs.appendFileSync( app_delegate_cpp, "\n\n#include \"Magpie.h\"\n\n// shared Magpie sington\nstatic Magpie s_sharedMagpie;\n\n" );
			echo( "patching AppDelegate.cpp ... ok");
			return true;
		}
		
		echo( "Error, file not found: " + app_delegate_cpp );
		return false;
	},
	replace_main_java: function() {
		var main_java = this.main_java;
		var files = find(this.droid_newdir + "/src").filter(function(file) { return file.endsWith(main_java); });
		if( files.length == 0 ) {
			echo( "Error: main java not found: " + main_java );
			return false;
		}
		
		var main_java_src =  this.magpie_dir + "/platforms/android/src/__AppName__.java";
		var main_java_dest = files[0];
		var source_code = fs.readFileSync( main_java_src + "", 'utf8' )
			.replace("com.handywit.__AppName__", this.package_id)
			.replace("__AppName__", this.app_name);
		
		fs.writeFileSync( main_java_dest, source_code, "utf8" );
		
		echo( "replacing main java: " + main_java_dest + " ... ok" );
		
		return true;
	},
	update_config_xml: function() {
		var files = [ this.proj_root + "/config.xml", 
		                this.droid_newdir + "/res/xml/config.xml",
		                this.ios_newdir + "/" + this.app_name + "/config.xml"
		               ];
		for(var i in files) {
			var content = fs.readFileSync( files[i], 'utf8' )
				.replace("com.handywit.__AppName__", this.package_id)
				.replace("__AppName__", this.app_name);
			fs.writeFileSync( files[i], content, "utf8" );
		}
		echo( "patching config.xml ... ok" );
		return true;
	},
	patch_android_mk: function() {
		var files = [ "MagpieBridgeJni.cpp", "../../Classes/Magpie.cpp" ];
		var str0 = "hellocpp/main.cpp";
		var strAdd = "";
		for(var i in files) {
			strAdd += " \\\n                   " + files[i];
		}
		var content = fs.readFileSync( this.proj_dir + "/proj.android/jni/Android.mk", 'utf8' )
			.replace(str0, str0 + strAdd);
		fs.writeFileSync( this.droid_newdir + "/jni/Android.mk", content, "utf8" );
		echo( "patching Android.mk ... ok" );
		return true;
	},
	patch_xcode_project_file: function() {
		echo( "\n--- patching xcode project file ..." );
		
		this.xcode_proj_file = this.ios_newdir + "/" + this.app_name + ".xcodeproj/project.pbxproj";
		echo( "target file: " + this.xcode_proj_file );

		this.xcode_proj_content = fs.readFileSync( this.xcode_proj_file, 'utf8' );
		this.cacheUUID = [];

		var info_plist_newpath = this.app_name + "/" + this.app_name + "-Info.plist";
		mv( this.ios_newdir + "/Info.plist", this.ios_newdir + "/" + info_plist_newpath);
		this.xcode_proj_content = this.xcode_proj_content
			.replace(/ Info.plist/g, " \"" + info_plist_newpath + "\"")
			.replace(/\.\.\/proj\.ios/g, "../ios");
		echo( "moving and renaming Info.plist ... ok");
		
		if( this.insertPluginsEntry() && this.insertFilesEntry() ) {
			fs.writeFileSync( this.xcode_proj_file, this.xcode_proj_content, "utf8" );
			echo( "writting xcode project files ... ok");
			return true;
		}
		
		return false;
	},
	generateUUID: function() {
	    var id = uuid.v4().replace(/-/g,'').substr(0,24).toUpperCase();

	    if (this.cacheUUID.indexOf(id) >= 0) {
	    	return this.generateUUID();

    	} else if (this.xcode_proj_content.indexOf(id) >= 0) {
    		this.cacheUUID.push( id );
        	return this.generateUUID();
    	}

    	this.cacheUUID.push( id );
        return id;
	},
	insertPluginsEntry: function() {
		var classes_tag = grep(/\/\* Classes \*\/,/, this.xcode_proj_file).replace("Classes","").replace(/[ \t\r\n\*\/,]*/g, "");
		
		var line1 = classes_tag + " /* Classes */,";
		var line2 = classes_tag + " /* Classes */ = {"

		var add_line1 = this.generateUUID() + " /* Plugins */,\n                                ";
		var add_line2 = this.generateUUID() + " /* Plugins */ = {\n\
		                        isa = PBXGroup;\n\
		                        children = (\n\
		                        );\n\
		                        name = Plugins;\n\
		                        path = " + this.app_name + "/Plugins;\n\
		                        sourceTree = SOURCE_ROOT;\n\
		                };\n                ";
		this.xcode_proj_content = this.xcode_proj_content
			.replace(line1, add_line1 + line1)
			.replace(line2, add_line2 + line2);
		
		echo("adding entry 'Plugins' ... ok");
		return true;
	},
	insertFilesEntry: function() {
		this.fileTypes = {
				".h": "sourcecode.c.h",
				".cpp": "sourcecode.cpp.cpp",
				".m": "sourcecode.c.objc",
				".mm": "sourcecode.cpp.objcpp",
				".framework" : "wrapper.framework"
			};

		return this.insertFile( ["Magpie.framework", 
		                         "System/Library/Frameworks/AssetsLibrary.framework", 
		                         "System/Library/Frameworks/MobileCoreServices.framework"], "Frameworks", "System/Library/Frameworks/Foundation.framework" ) &&
		                         
			this.insertFile( ["MagpieBridgeiOS.h", 
			                  "MagpieBridgeiOS.mm"], "Sources", "AppController.mm" ) &&
			                  
			this.insertFile( ["../Classes/Magpie.h", 
			                  "../Classes/Magpie.cpp"], "Sources", "../Classes/AppDelegate.cpp" );
	},
	insertFile: function( filepaths, group, beforePath ) {
		var beforeFile = path.basename( beforePath );
		var beforeFileInGroup = beforeFile + " in " + group;
		var beforeFileType = this.fileTypes[ path.extname( beforeFile ) ];
		var patterns = [
		              "\\/\\* " + beforeFile + " \\*\\/,",
		              "\\/\\* " + beforeFileInGroup + " \\*\\/,",
		              "\\/\\* " + beforeFile + " \\*\\/ = {",
		              "\\/\\* " + beforeFileInGroup + " \\*\\/ = {"
		              ]
		var lines = [];
		for(var i=0; i<patterns.length; i++) {
			var re = new RegExp(patterns[i]);
			var line = grep( re, this.xcode_proj_file );
			lines.push( line );
		}
		var beforeFileTag = lines[0].replace(beforeFile,"").replace(/[ \t\r\n\*\/,]*/g, "");
		var beforeGroupTag = lines[1].replace(beforeFileInGroup,"").replace(/[ \t\r\n\*\/,]*/g, "");
		if(beforeFileTag.length == 0 || beforeGroupTag.length == 0) {
			echo( "Error: entry not found: " + beforePath );
			return false;
		}
		
		var insertLines = ["", "", "", ""];
		for(var i in filepaths) {
			var addPath = filepaths[i];
			if(this.xcode_proj_content.indexOf(addPath) > 0) continue;
			
			var addFile = path.basename( addPath );
			var addFileInGroup = addFile + " in " + group;
			var addFileType = this.fileTypes[ path.extname(addFile) ];
			var addFileTag = this.generateUUID();
			var addGroupTag = this.generateUUID();
			
			for(var j in lines) {
				var addLine = lines[ j ]
					.replace(beforeFileTag, addFileTag)
					.replace(beforeGroupTag, addGroupTag)
					.replace(beforeFileInGroup, addFileInGroup)
					.replace("path = " + beforePath, "path = " + addPath)
					.replace(beforeFileType, addFileType)
					.replace(beforeFile, addFile)
					.replace(beforeFile, addFile);
				if(addLine.indexOf("Magpie.framework") >= 0) {
					addLine = addLine
						.replace("SDKROOT", "SOURCE_ROOT")
						.replace("name = Magpie.framework; ", "");
					
					if(this.xcode_proj_content.indexOf("FRAMEWORK_SEARCH_PATHS =") < 0) {
						var str_buildsettings = "buildSettings = {";
						var add_search_paths = "\nFRAMEWORK_SEARCH_PATHS = ( \n\"$(inherited)\", \n\"$(PROJECT_DIR)\", );";
						this.xcode_proj_content = this.xcode_proj_content.replace( str_buildsettings, str_buildsettings + add_search_paths );
					}
					
				}
				insertLines[j] += addLine;
			}
			
			echo( "adding entry '" + addPath + "' to '" + group + "' ... ok");
		}
		
		for(var j in lines) {
			this.xcode_proj_content = this.xcode_proj_content.replace(lines[j], insertLines[j] + lines[j]);
		}
		
		return true;
	}
};

magpie_cli.main( process.argv );

