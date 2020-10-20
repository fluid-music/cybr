// @ts-check

const path = require('path')
const fluid = require('../fluid-music')
const cybr = fluid.cybr
const chords = require('./midi-chords')

const tyrellN6 = new fluid.TyrellN6Vst2({
  // Osc Mod
  tyrellSoftSync: 100,

  // Envelope 1
  env1Attack: 0,
  env1Decay: 50,
  env1Sustain: 9.5,
  env1Release: 18,

  // Envelope 2
  env2Attack: 0,
  env2Decay: 23.6,
  env2Sustain: 6,
  env2Release: 20,

  env1Velocity: 0,
  env2Velocity: 80,

  // LP/HP filter
  tyrellCutoff: 60,
  tyrellResonance: 34,
  tyrellKeyFollow: 100,
  tyrellFreqModDepth1: 120,
  tyrellFreqModSrc2: fluid.TyrellN6Vst2.parameterLibrary.tyrellFreqModSrc2.choices.velocity,
  tyrellFreqModDepth2: 18,

  // Oscillators
  tyrellOscVolume1: -30,
  tyrellOscVolume2: -78.5,
  tyrellSubVolume: -91,
  tyrellTune2: 12,

  tyrellShape1: 2.35,
  tyrellShape2: 1.7,

  // Osc Mod
  tyrellPwdepth: 15
})
tyrellN6.automation.tyrellTune2 = { points: [{ value: tyrellN6.parameters.tyrellTune2, startTime: 0, curve: 0 }] }
tyrellN6.automation.tyrellCutoff = { points: [{ value: tyrellN6.parameters.tyrellCutoff, startTime: 0, curve: 0 }] }

const v = new fluid.techniques.PluginAuto(tyrellN6.makeAutomation.tyrellTune2(1))
const V = new fluid.techniques.PluginAuto(tyrellN6.makeAutomation.tyrellCutoff(65))
const w = [v, V]

class MidiArp extends fluid.techniques.MidiChord {
  constructor (options) {
    super(options)
    this.addFadeInOut = !!options.addFadeInOut
  }

  /**
   * @param {number} startTime
   * @param {number} duration
   * @notparam {import('fluid-music/built/ts-types').ClipEventContext} context
   */
  use (startTime, duration, context) {
    const stepSize = 1 / 4 / 8
    const numSteps = Math.floor(duration / stepSize)

    for (let i = 0; i < numSteps; i++) {
      const note = this.notes[i % this.notes.length]
      const velocity = Math.min(127, Math.max(1, Math.round(80 * Math.pow(1.045, -i))))
      const noteStartTime = stepSize * i + startTime
      const noteDuration = stepSize
      const technique = new fluid.techniques.MidiNote({ note, velocity })
      technique.use(noteStartTime, noteDuration, context)
    }

    // Add fade in and fade out. Remember that the times below are relative to
    // the clip (not the session)
    const strTime = 0
    const endTime = context.clip.duration
    const midTime = endTime / 2
    const auto1 = new fluid.techniques.TrackAuto({ paramKey: 'gain', curve: -0.5, value: -Infinity })
    const auto2 = new fluid.techniques.TrackAuto({ paramKey: 'gain', curve: 0.5, value: 0 })
    const auto3 = new fluid.techniques.TrackAuto({ paramKey: 'gain', curve: 0.0, value: -Infinity })
    auto1.use(strTime, 0, context)
    auto2.use(midTime, 0, context)
    auto3.use(endTime, 0, context)

    return null
  }
}

const techniques = chords.map(chord => new MidiArp(chord))
const tLibrary = fluid.tLibrary.fromArray(techniques)

const session = new fluid.FluidSession({
  bpm: 92,
  r: 'hhh',
  tLibrary: Object.assign({ v, V, w }, tLibrary)
}, {
  chords1: { plugins: [tyrellN6] },
  chords2: { plugins: [new fluid.TyrellN6Vst2(tyrellN6.parameters)] },
  chords3: { plugins: [new fluid.TyrellN6Vst2(tyrellN6.parameters)] }
})

const r3 = { r: '123', clips: ['...'] }
const r5 = { r: '12345', clips: ['.....'] }
const rest6 = { r: 'hhh', clips: ['...'] }

session.insertScore({
  chords1: {
    clips: ['a--', 'd--', 'f--'],
    chords1: ['...', 'w'],
    eventMappers: []
  },
  chords2: {
    clips: [r3, 'b--', 'w', 'e--']
  },
  chords3: {
    clips: [r5, 'c--']
  }
}, {})

// pseudo side-chain first track
const clips2and3 = [
  ...session.tracks.find(t => t.name === 'chords2').clips,
  ...session.tracks.find(t => t.name === 'chords3').clips]
  .filter(clip => clip.midiEvents && clip.midiEvents.length)
const timesAndAmounts = clips2and3.map(c => [
  { time: c.startTime, delta: 0 },
  { time: c.startTime + c.duration / 2, delta: -8 },
  { time: c.startTime + c.duration, delta: 8 }
]).flat()
let tds = []
timesAndAmounts.forEach(td => {
  const obj = tds.find(td2 => td2.time === td.time)
  if (obj) {
    obj.delta += td.delta
  } else {
    tds.push(td)
  }
})
tds = tds.sort((a, b) => a.time - b.time)
let sum = 0
const gainAutomation = tds.map((td, i) => {
  sum += td.delta
  return { type: 'trackAuto', paramKey: 'gain', startTime: td.time, value: sum, curve: -0.5 }
})
session.tracks.find(track => track.name === 'chords1').automation.gain = { points: gainAutomation }

// Convert to reaper/tracktion session
const client = new cybr.Client({ timeout: Math.pow(2, 31) - 1 })
client.connect(true)

const run = async () => {
  // create RPP
  const rpp = await fluid.sessionToReaperProject(session, client)
  console.log(rpp.dump())

  // send to CYBR
  const activateMsg = fluid.cybr.global.activate(path.join(__dirname, 'demo-shepherd.tracktionedit'), true)
  const templateMsg = fluid.sessionToTemplateFluidMessage(session)
  const tracksMsg = fluid.sessionToFluidMessage(session)
  const saveMsg = fluid.cybr.global.save()

  await client.send([activateMsg, templateMsg, tracksMsg, saveMsg])
}

run().finally(() => {
  client.close()
})
