import { bashSync, bashAsync } from './build_utils.mjs';

async function build() {

	bashSync("rm -rf payload_x64.dll");
	bashSync("rm -rf payload_x32.dll");

	let CC=""

	let DEFINES=""
	let LIBS=""

	let SRC_FILES=[]
	SRC_FILES.push("src_payload/dllmain.c")
	SRC_FILES.push("src_payload/speedhack.c")
	SRC_FILES.push("src_payload/cnc_window.c")
	SRC_FILES.push("src_payload/tls.c")

	SRC_FILES.push("third_party/minhook/buffer.c")
	SRC_FILES.push("third_party/minhook/hook.c")
	SRC_FILES.push("third_party/minhook/trampoline.c")
	SRC_FILES.push("third_party/minhook/hde/hde32.c")
	SRC_FILES.push("third_party/minhook/hde/hde64.c")

	// TODO -O3 -s
	CC="gcc -Wall -Wextra"

	async function compile_c(c_file, arch) {
		let obj_path = "_obj_speedhack/" + c_file.replace(/\//g, "_").replace(/\\/g, "_").replace(/\./g, "_") + arch + ".o";
		let compile_cmd = `${CC} ${DEFINES} -I. ${c_file} -c -o ${obj_path} -${arch} -Wfatal-errors`;
		// console.log(compile_cmd);
		let res = await bashAsync(compile_cmd);
		// console.log(res);
		return obj_path;
	}

	bashSync(`rm -rf _obj_speedhack/*.m32.o`)
	bashSync(`rm -rf _obj_speedhack/*.m64.o`)
	bashSync(`mkdir -p _obj_speedhack`)

	let m32_promises = [];
	let m64_promises = [];

	for (let c_file of SRC_FILES) { m32_promises.push(compile_c(c_file, "m32")); }
	for (let c_file of SRC_FILES) { m64_promises.push(compile_c(c_file, "m64")); }

	let m32_obj_speedhackect_files = await Promise.all(m32_promises);
	let m64_obj_speedhackect_files = await Promise.all(m64_promises);

	console.log("Linking m32...");
	bashSync(`${CC} -shared ${DEFINES} payload_x32.def -odata/payload_x32.dll -m32 -Wfatal-errors -L. ${LIBS} ${m32_obj_speedhackect_files.join(" ")}`);

	console.log("Linking m64...");
	bashSync(`${CC} -shared ${DEFINES} payload_x64.def -odata/payload_x64.dll -m64 -Wfatal-errors -L. ${LIBS} ${m64_obj_speedhackect_files.join(" ")}`);

	console.log("Payload build complete");

	bashSync(`${CC} ${DEFINES} -odata/inject_x64.exe -m64 -Wfatal-errors -L. ${LIBS} src_injector/inject.c`);
	bashSync(`${CC} ${DEFINES} -odata/inject_x32.exe -m32 -Wfatal-errors -L. ${LIBS} src_injector/inject.c`);

	// bashSync("");

	// bashSync(``);

}

build().catch(e=>{
	console.log(e);
	process.exit(1);
})