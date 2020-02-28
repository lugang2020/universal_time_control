import { bashSync, bashAsync, runAsyncParallel } from './build_utils.mjs';

async function build() {

	bashSync("rm -f ui_impl.dll");

	let SEPPLES="g++ -Wall -Wextra -g --std=c++17"

	let DEFINES=``
	let LIBS="-lGdi32 -ld3d9"

	let SRC_FILES=[]

	SRC_FILES.push("third_party/cimgui/cimgui.cpp")
	SRC_FILES.push("third_party/cimgui/imgui/imgui.cpp")
	SRC_FILES.push("third_party/cimgui/imgui/imgui_draw.cpp")
	SRC_FILES.push("third_party/cimgui/imgui/imgui_demo.cpp")
	SRC_FILES.push("third_party/cimgui/imgui/imgui_widgets.cpp")
	SRC_FILES.push("src_ui_impl/gui_impl_windx9.cpp")
	SRC_FILES.push("third_party/imgui_dx9/imgui_impl_dx9.cpp")
	SRC_FILES.push("third_party/imgui_dx9/imgui_impl_win32.cpp")



	async function compile_c(c_file) {
		let obj_path = "_obj_ui_impl/" + c_file.replace(/\//g, "_").replace(/\\/g, "_").replace(/\./g, "_") + ".o";
		let compile_cmd = `${SEPPLES} ${DEFINES} -Ithird_party -Ithird_party/imgui_soft/emilib -Ithird_party/cimgui ${c_file} -c -o ${obj_path} -m32 -Wfatal-errors`;
		// console.log(compile_cmd);
		let res = await bashAsync(compile_cmd);
		// console.log(res);
		return obj_path;
	}

	bashSync(`rm -rf _obj_ui_impl`)
	bashSync(`mkdir _obj_ui_impl`)

	// let promises = [];

	// for (let c_file of SRC_FILES) {
	// 	promises.push(compile_c(c_file));
	// }

	// let object_files = await Promise.all(promises);
	// console.log(object_files);

	let to_run = [];
	for (let c_file of SRC_FILES) {
		to_run.push(compile_c.bind(this, c_file));
	}

	let object_files = await runAsyncParallel(to_run, 8);


	console.log("Linking...");
	bashSync(`${SEPPLES} -m32 -shared -o ui_impl.dll -Wl,--stack,16777216 ${object_files.join(" ")} -L. ${LIBS}`);

	console.log("Build complete");
	// bashSync("");

	// bashSync(``);

}

build().catch(e=>{
	console.log(e);
	process.exit(1);
})