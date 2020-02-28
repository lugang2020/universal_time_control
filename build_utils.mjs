import { spawn, spawnSync } from 'child_process';

export function bashSync(str) {
	let result = spawnSync("C:/Git/usr/bin/bash.exe", ["-c", str], { "stdio": "inherit" });
	if (result.status !== 0) process.exit(1);
	return result;
}

export function bashAsync(str) { return new Promise((resolve, reject) => {
	let child_process = spawn("C:/Git/usr/bin/bash.exe", ["-c", str], { "stdio": "inherit" });
	child_process.on("exit", (code, signal) => { if (code == 0) resolve(); else reject(); });
}); }

export async function runAsyncParallel(to_run, max_parallel) {

	const ret = [];
	const executing = [];

	for (const item of to_run) {
		const p = item();//Promise.resolve().then(() => iteratorFn(item, array));
		ret.push(p);
		const e = p.then(() => executing.splice(executing.indexOf(e), 1));
		executing.push(e);
		if (executing.length >= max_parallel) {
			await Promise.race(executing);
		}
	}
	return Promise.all(ret);

}