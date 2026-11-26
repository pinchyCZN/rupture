/*
 * OURS. The browser half of core/music.cpp, linked with --js-library:
 * Web Audio's decodeAudioData on the preloaded MP3 bytes, a gain node
 * for the volume, and an AudioContext the browser keeps suspended until
 * a key or a click resumes it.
 */
mergeInto(LibraryManager.library, {
	music_web_load: function(p, n) {
		var AC = window.AudioContext || window.webkitAudioContext;
		var M = { ctx: null, buf: null, gain: null, src: null, t0: 0, off: 0, paused: false };
		var resume = function() {
			if (M.ctx.state !== 'running' && !M.paused) {
				M.ctx.resume();
			}
		};

		Module.ruptureMusic = M;
		if (!AC) {
			return;
		}
		M.ctx = new AC();
		M.gain = M.ctx.createGain();
		M.gain.gain.value = 0;
		M.gain.connect(M.ctx.destination);
		M.ctx.decodeAudioData(HEAPU8.slice(p, p + n).buffer, function(b) { M.buf = b; }, function() {});
		window.addEventListener('keydown', resume);
		window.addEventListener('pointerdown', resume);
	},

	music_web_play: function(at) {
		var M = Module.ruptureMusic;

		if (!M || !M.ctx) {
			return 0;
		}
		if (M.src) {
			return 1;
		}
		if (!M.buf || M.ctx.state !== 'running' || at >= M.buf.duration) {
			return 0;
		}
		M.src = M.ctx.createBufferSource();
		M.src.buffer = M.buf;
		M.src.connect(M.gain);
		M.t0 = M.ctx.currentTime;
		M.off = at;
		M.src.start(0, at);
		return 1;
	},

	music_web_seconds: function() {
		var M = Module.ruptureMusic;

		if (!M || !M.src) {
			return -1;
		}
		return Math.min(M.ctx.currentTime - M.t0 + M.off, M.buf.duration);
	},

	music_web_length: function() {
		var M = Module.ruptureMusic;

		return M && M.buf ? M.buf.duration : 0;
	},

	music_web_volume: function(v) {
		var M = Module.ruptureMusic;

		if (M && M.gain) {
			M.gain.gain.value = v;
		}
	},

	music_web_pause: function(on) {
		var M = Module.ruptureMusic;

		if (!M || !M.ctx) {
			return;
		}
		M.paused = on !== 0;
		if (M.paused) {
			M.ctx.suspend();
		} else {
			M.ctx.resume();
		}
	},

	music_web_seek: function(at) {
		var M = Module.ruptureMusic;

		if (!M || !M.ctx || !M.buf || !M.src || at >= M.buf.duration) {
			return;
		}
		M.src.stop();
		M.src.disconnect();
		M.src = M.ctx.createBufferSource();
		M.src.buffer = M.buf;
		M.src.connect(M.gain);
		M.t0 = M.ctx.currentTime;
		M.off = at;
		M.src.start(0, at);
	}
});
