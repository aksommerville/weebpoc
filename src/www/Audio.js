const insv = [ // [attackTime, peakLevel, decayTime, sustainLevel, releaseTime, wave]
  [0.010, 0.230, 0.020, 0.080, 0.300, 0.5, "0963"],            // 0: Punchy Bass
  [0.010, 0.170, 0.025, 0.090, 0.500, [1,1,0,0.5,0,0.3,0,0.2,0,0.1]], // 1: Simple Square
  [0.040, 0.230, 0.030, 0.090, 0.800, [1,1,0.5,0.3,0.2,0.05]], // 2: Violin
  [0.010, 0.230, 0.020, 0.090, 0.900, 7.6, "9990"],            // 3: Bell
  [0.025, 0.230, 0.040, 0.110, 0.500, [1,1,0,0.2,0,0.07]],     // 4: Clean organ
  [0.015, 0.230, 0.025, 0.050, 0.900, 2, "0972"],              // 5: Punchy Lead
  // 6..15 are available to define.
];

export class Audio {
  constructor() {
    if (!AudioContext) return;
    this.hz = [];
    for (let n=0; n<0x80; n++) {
      this.hz.push(440 * Math.pow(2, (n - 69) / 12));
    }
    this.ctx = new AudioContext({
      sampleRate: 44100,
      latencyHint: "interactive",
    });
    if (this.ctx.state === "suspended") {
      this.ctx.resume();
    }
    this.songs = {}; // Key=name, value=text
    this.songTempo = 1; // ms/tick
    this.song = "";
    this.songp = 0;
    this.songDelay = 0; // s
    /*IGNORE{*/
    this.loadUncompiledSongs();
    if (false)
    /*}IGNORE*/
    this.loadCompiledSongs();
  }
  
  end() {
    if (this.ctx) {
      this.ctx.suspend();
      this.ctx = null;
    }
  }
  
  update() {
    if (!this.ctx) return;
    
    if (this.song) {
      const limit = this.ctx.currentTime + 3.000; // Read ahead by so much, into the song.
      while (this.songTime < limit) {
        if (this.songp >= this.song.length) {
          this.songp = 1;
          this.songTime += 0.010; // Advance a little bit on looping, otherwise a faulty song could kill us.
          continue;
        }
        const opcode = this.song.charCodeAt(this.songp++);
        if (opcode === 0x27) { // Delay.
          const tickc = this.song.charCodeAt(this.songp++) - 0x3e;
          if ((tickc < 1) || (tickc > 0x40)) return this.playSongSerial("");
          this.songTime += (tickc * this.songTempo) / 1000;
        } else { // 0x28..0x37 are Notes, for the respective channel.
          const chid = opcode - 0x28;
          if ((chid < 0) || (chid > 15)) return this.playSongSerial("");
          const noteid = 0x23 + this.song.charCodeAt(this.songp++) - 0x3f;
          if ((noteid < 0x23) || (noteid > 0x62)) return this.playSongSerial("");
          const duration = this.song.charCodeAt(this.songp++) - 0x3f;
          if ((duration < 0) || (duration >= 0x40)) return this.playSongSerial("");
          this.playNote(chid, noteid, (duration * this.songTempo) / 1000, this.songTime);
        }
      }
    }
  }
  
  playNote(chid, noteid, durs, when) {
    //console.log(`Audio.playNote chid=${chid} noteid=${noteid} durs=${durs} when=${when}`);
    
    // gloom_for_company: 1,2 [organ,bass]
    // eye_of_newt: 0,1,2,3,4,5 [bass,powerchords,lead1,lead2(A rsp),lead3(chords),lead4(B)]
    // feline_mind: 0,1 [lead,bass]

    const ins = insv[chid];
    if (!ins) return;
    let fmrate = 0;
    const opt = { frequency: this.hz[noteid] };
    if (ins[5] instanceof Array) ins[5] = new PeriodicWave(this.ctx, { real: ins[5] });
    if (ins[5] instanceof PeriodicWave) opt.periodicWave = ins[5];
    else if (typeof(ins[5]) === "string") opt.type = ins[5];
    else if (typeof(ins[5]) === "number") { opt.type = "sine"; fmrate = ins[5]; }
    else return;
    const osc = new OscillatorNode(this.ctx, opt);
    const t0 = when;
    const ta = t0 + ins[0];
    const va = ins[1];
    const td = ta + ins[2];
    const vd = ins[3];
    const ts = td + durs;
    const tr = ts + ins[4];
    let modgain, mod;
    if (fmrate) {
      modgain = new GainNode(this.ctx);
      if (ins[6]) {
        const a = (opt.frequency * ins[6][0].toString()) / 10;
        const b = (opt.frequency * ins[6][1].toString()) / 10;
        const c = (opt.frequency * ins[6][2].toString()) / 10;
        const d = (opt.frequency * ins[6][3].toString()) / 10;
        modgain.gain.setValueAtTime(a, 0);
        modgain.gain.setValueAtTime(a, t0);
        modgain.gain.setValueAtTime(b, ta);
        modgain.gain.setValueAtTime(c, td);
        modgain.gain.setValueAtTime(c, ts);
        modgain.gain.setValueAtTime(d, tr);
      } else {
        modgain.gain.value = opt.frequency;
      }
      mod = new OscillatorNode(this.ctx, { type: "sine", frequency: opt.frequency * fmrate });
      mod.start();
      mod.connect(modgain);modgain.connect(osc.frequency);
    }
    const env = new GainNode(this.ctx);
    env.gain.setValueAtTime(0, 0);
    env.gain.setValueAtTime(0, t0);
    env.gain.linearRampToValueAtTime(va, ta);
    env.gain.linearRampToValueAtTime(vd, td);
    env.gain.setValueAtTime(vd, ts);
    env.gain.linearRampToValueAtTime(0, tr);
    osc.connect(env);
    env.connect(this.ctx.destination);
    osc.start();
    setTimeout(() => {
      env.disconnect();
      osc.stop();
      if (modgain) modgain.disconnect();
      if (mod) { mod.stop(); mod.disconnect(); }
    }, (tr - this.ctx.currentTime) * 1000);
  }
  
  playSong(name) {
    if (!name) {
      this.playSongSerial("");
    } else if (this.songs[name]) {
      this.playSongSerial(this.songs[name]);
    } else {
      this.pendingSongName = name;
    }
  }
  
  playSongSerial(src) {
    if (src && (src.length > 1) && this.ctx) {
      this.song = src;
      this.songp = 1;
      this.songTempo = src.charCodeAt(0) - 0x3e;
      this.songTime = this.ctx.currentTime;
    } else {
      this.song = "";
      this.songp = 0;
      this.songTempo = 1;
      this.songTime = 0;
    }
  }
  
  loadCompiledSongs() {
    for (const e of document.querySelectorAll("song")) {
      const src = e.innerText.replace(/\s+/g, "");
      const name = e.getAttribute("name");
      this.songs[name] = src;
    }
  }
  
  /* Everything below this point only exists pre-minification, and duplicates the minifier's song conversion logic.
   ***************************************************************************************************/
  
  /*IGNORE{*/
  loadUncompiledSongs() {
    Promise.all(Array.from(document.querySelectorAll("song")).map(e => {
      const url = e.getAttribute("href");
      const name = url.match(/^.*\/([^/.]*)\..*$/)[1];
      return fetch(url).then(rsp => {
        if (!rsp.ok) throw rsp;
        return rsp.arrayBuffer();
      }).then(serial => {
        const song = this.songFromMidi(serial, name);
        this.songs[name] = song;
        if (this.pendingSongName === name) {
          delete this.pendingSongName;
          this.playSongSerial(song);
        }
      });
    })).then(() => {
    }).catch(e => {
      console.error(`...failed to load songs`, e);
    });
  }
  
  songFromMidi(src, name) {
    const tempo = 10; // ms/tick. Not going to overthink it, just use a constant.
    const opcodeAlphabet = "'()*+,-./01234567"; // delay,ch0,...,ch15
    const dataAlphabet = "?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"; // 0..63
    const dst = [dataAlphabet[tempo - 1]]
    const reader = new MidiFileReader(src);
    let delay = 0;
    let when = 0;
    const notesHeld = []; // {chid,noteid,durp,when}
    const flushDelay = () => {
      let delayTicks = Math.round((delay * 1000) / tempo);
      if (delayTicks > 0) {
        delay -= (delayTicks * tempo) / 1000;
        while (delayTicks > 0x40) {
          dst.push("'" + dataAlphabet[0x3f]);
          delayTicks -= 0x40;
        }
        if (delayTicks > 0) {
          dst.push("'" + dataAlphabet[delayTicks - 1]);
        }
      }
    };
    while (!reader.isFinished()) {
    
      const newDelay = reader.getDelay();
      when += newDelay;
      delay += newDelay;
      // Don't emit the delay yet -- we're going to discard events and we don't want to end up with back-to-back delays.
      
      let event;
      while (event = reader.getEvent()) {
        const opcode = event[0] & 0xf0;
        
        if (opcode === 0x80) { // Note Off
          const chid = event[0] & 0x0f;
          const noteid = event[1] - 0x23;
          if ((noteid < 0) || (noteid >= 0x40)) continue;
          const heldp = notesHeld.findIndex(n => ((n.chid === chid) && (n.noteid === noteid)));
          if (heldp < 0) continue;
          const hold = notesHeld[heldp];
          notesHeld.splice(heldp, 1);
          const duration = when - hold.when;
          let durationTicks = Math.round((duration * 1000) / tempo);
          if (durationTicks < 0) durationTicks = 0;
          else if (durationTicks >= 0x40) durationTicks = 0x3f;
          dst[hold.durp] = dataAlphabet[durationTicks];
        
        } else if (opcode === 0x90) { // Note On
          const chid = event[0] & 0x0f;
          const noteid = event[1] - 0x23;
          if ((noteid < 0) || (noteid >= 0x40)) {
            console.error(`song ${name}: Illegal note id ${event[1]} on channel ${chid}. Must be in 35..98`);
            continue;
          }
          // event[2] velocity is not used.
          flushDelay();
          dst.push(opcodeAlphabet[1 + chid] + dataAlphabet[noteid]);
          const durp = dst.length;
          dst.push(dataAlphabet[0]);
          notesHeld.push({ chid, noteid, durp, when });
          
        } // No other event is meaningful. Nice and easy!  
      }
    }
    flushDelay();
    return dst.join("");
  }
  /*}IGNORE*/
}

/*IGNORE{*/
class MidiFileReader {
  constructor(serial) {
    this.src = new Uint8Array(serial);
    this.decode();
  }
  
  isFinished() {
    for (const track of this.tracks) {
      if (track.p < track.src.length) return false;
    }
    return true;
  }
  
  /* Return any pending delay in seconds, and advance my internal clock by that much.
   */
  getDelay() {
    let shortest = 0x40000000; // Not representable as VLQ, definitely larger than any real answer.
    for (const track of this.tracks) {
      this.requireDelay(track);
      if (track.p >= track.src.length) continue;
      if (track.delay < shortest) shortest = track.delay; // Including zero.
    }
    if (shortest >= 0x40000000) return 0;
    for (const track of this.tracks) {
      if (track.p >= track.src.length) continue;
      track.delay -= shortest;
    }
    return (this.usperqnote * shortest) / (this.division * 1000000);
  }
  
  /* Return any pending event at the current readhead, or null if none (EOF or delay pending).
   * Events are an array of 0..255.
   * Meta and Sysex events do not contain their payload length, you can infer it from the array length.
   */
  getEvent() {
    for (const track of this.tracks) {
      this.requireDelay(track);
      if (track.delay) continue;
      if (track.p >= track.src.length) continue;
      const event = this.readEvent(track);
      this.localEvent(event, track);
      return event;
    }
    return null;
  }
  
  localEvent(event, track) {
    if ((event[0] === 0xff) && (event[1] === 0x51) && (event.length === 5)) { // Set Tempo
      this.usperqnote = (event[2] << 16) | (event[3] << 8) | event[4];
    } else if ((event[0] === 0xff) && (event[1] === 0x2f)) { // End Of Track
      track.p = track.src.length;
    }
  }
  
  requireDelay(track) {
    if (track.delay >= 0) return;
    if (track.p >= track.src.length) return;
    track.delay = 0;
    while (track.src[track.p] & 0x80) {
      track.delay <<= 7;
      track.delay |= track.src[track.p++] & 0x7f;
    }
    track.delay <<= 7;
    track.delay |= track.src[track.p++];
  }
  
  readEvent(track) {
    if (track.p >= track.src.length) return [0xff, 0x2f];
    track.delay = -1;
    let status = track.src[track.p];
    if (status & 0x80) {
      track.p++;
    } else if (track.status & 0x80) {
      status = track.status;
    } else {
      throw new Error(`Unexpected leading byte ${track.src[track.p]} at ${track.p}/${track.src.length} in MTrk`);
    }
    track.status = status;
    const data1 = () => [status, track.src[track.p++]];
    const data2 = () => [status, track.src[track.p++], track.src[track.p++]];
    switch (status & 0xf0) {
      case 0x80: return data2(); // Note Off
      case 0x90: { // Note On
          const event = data2();
          if (!event[2]) { // Note On with velocity zero is Note Off.
            event[0] = 0x80;
            event[2] = 0x40;
          }
          return event;
        }
      case 0xa0: return data2(); // Note Adjust
      case 0xb0: return data2(); // Control Change
      case 0xc0: return data1(); // Program Change
      case 0xd0: return data1(); // Channel Pressure
      case 0xe0: return data2(); // Pitch Wheel
      case 0xf0: { // Meta or Sysex
          track.status = 0;
          const event = [status];
          if (status === 0xff) { // Meta has a "type" byte, beyond that Meta and Sysex are the same thing.
            event.push(track.src[track.p++]);
          }
          let paylen = 0;
          while (track.src[track.p] & 0x80) {
            paylen <<= 7;
            paylen |= track.src[track.p++] & 0x7f;
          }
          paylen <<= 7;
          paylen |= track.src[track.p++];
          while (paylen-- > 0) {
            event.push(track.src[track.p++]);
          }
          return event;
        }
    }
    return [0xff, 0x2f];
  }
  
  decode() {
    this.usperqnote = 500000;
    this.division = 0;
    this.tracks = [];
    for (let srcp=0; srcp<this.src.length; ) {
      let id = this.src[srcp++] << 24;
      id |= this.src[srcp++] << 16;
      id |= this.src[srcp++] << 8;
      id |= this.src[srcp++];
      let len = this.src[srcp++] << 24;
      len |= this.src[srcp++] << 16;
      len |= this.src[srcp++] << 8;
      len |= this.src[srcp++];
      if (id === 0x4d546864) { // MThd
        this.division = (this.src[srcp+4] << 8) | this.src[srcp+5];
      } else if (id === 0x4d54726b) { // MTrk
        this.tracks.push({
          src: new Uint8Array(this.src.buffer, this.src.byteOffset + srcp, len),
          p: 0,
          delay: -1, // ticks, <0 if we need to read it
          status: 0,
        });
      }
      srcp += len;
    }
    if (!this.division || (this.division >= 0x8000) || !this.tracks.length) throw new Error(`Invalid MIDI file`);
  }
}
/*}IGNORE*/
