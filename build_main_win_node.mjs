import { bashSync, bashAsync, runAsyncParallel } from './build_utils.mjs';

async function build() {

	bashSync("rm -rf test_win.exe");

	bashSync(`rm -f config.db`)
	await bashAsync(`./sqlite3.exe config.db < data/db_001.sql`)

	// let LIVE_RELOAD = 1;

	let cimgui_promise = null;
	// cimgui_promise = bashAsync(`./build_cimgui_win.sh`)

	bashSync(`windres --input utc.rc --output utc.res --output-format=coff`);

	
	//-Wl,-subsystem,windows
	//utc.res \

	let CC="gcc -Wall -Wextra -g";
	let SEPPLES="g++ -Wall -Wextra -g --std=c++17"; // -fno-exceptions

	let DEFINES=""
	let LIBS="-lui_impl -lsqlite3 -lWtsapi32 -lutf8proc"

	let SRC_FILES_C=[]
	let SRC_FILES_CPP=[]

	SRC_FILES_C.push("src_main/entry_point.c")

	SRC_FILES_C.push("src_main/db/db_owner.c")

	SRC_FILES_C.push("src_main/gui/gui_entry_point.c")
	SRC_FILES_C.push("src_main/gui/gui_common.c")

	SRC_FILES_C.push("third_party/tinycthread/tinycthread.c")
	SRC_FILES_C.push("third_party/vec/vec.c")


	SRC_FILES_C.push("src_main/task_que/task_que.c")
	
	SRC_FILES_C.push("src_main/process_monitoring/process_inject.c")

	SRC_FILES_C.push("src_main/util.c")


	SRC_FILES_CPP.push("src_main/controls/control_manager.cpp")
	SRC_FILES_CPP.push("src_main/games/game_manager.cpp")
	SRC_FILES_CPP.push("src_main/process_monitoring/process_monitor.cpp")


	if (!process.env.LIVE_RELOAD) {
		SRC_FILES_C.push("src_main/gui/gui_util.c")
		SRC_FILES_C.push("src_main/gui/gui_mainloop.c")
		SRC_FILES_C.push("src_main/gui/gui_games_list.c")
		SRC_FILES_C.push("src_main/gui/gui_controls.c")
	} else {
		// CC="tcc -Wall -g -IC:/Code/winapi-full-for-0.9.27/include -IC:/Code/winapi-full-for-0.9.27/include/winapi"
		DEFINES+=" -DLIVE_RELOAD"
		LIBS+=" -ltcc -luser32"
	}

	async function compile_c(c_file, is_sepples) {
		let obj_path = "_obj_main/" + c_file.replace(/\//g, "_").replace(/\\/g, "_").replace(/\./g, "_") + ".o";
		let compile_cmd = `${is_sepples ? SEPPLES : CC} ${DEFINES} -I. ${c_file} -c -o ${obj_path} -m32 -Wfatal-errors`;
		let res = await bashAsync(compile_cmd);
		return obj_path;
	}

	bashSync(`rm -rf _obj_main`)
	bashSync(`mkdir _obj_main`)

	// let promises = [];

	// for (let c_file of SRC_FILES_C) {
	// 	promises.push(compile_c(c_file));
	// }

	// let object_files = await Promise.all(promises);
	// console.log(object_files);

	let to_run = [];
	for (let c_file of SRC_FILES_C) {
		to_run.push(compile_c.bind(this, c_file, false));
	}

	for (let cpp_file of SRC_FILES_CPP) {
		to_run.push(compile_c.bind(this, cpp_file, true));
	}


	let object_files = await runAsyncParallel(to_run, 8);

	if (cimgui_promise) {
		console.log("waiting for cimgui build...");
		await cimgui_promise;
		console.log("cimgui built ok");
	}

	console.log("Linking...");
	bashSync(`${SEPPLES} utc.res ${DEFINES} -Wl,--stack,16777216 -otest_win.exe -m32 -Wfatal-errors -L. ${LIBS} ${object_files.join(" ")}`);

	console.log("Build complete");
	// bashSync("");

	// bashSync(``);

}

build().catch(e=>{
	console.log("build_main:");
	console.log(e);
	console.trace(e);
	process.exit(1);
})