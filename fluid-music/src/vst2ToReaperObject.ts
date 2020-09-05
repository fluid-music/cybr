import { FluidPlugin, PluginType } from './plugin';
import FluidIpcClient = require('./cybr/IpcClient');
import * as cybr from './cybr/index';

const rppp = require('rppp')

/**
 * Create a `ReaperVst` object from an existing plugin on the cybr instance. 
 * This function changes the state of the currently-activated session on cybr, because it 
 * has to select every plugin to get its state.
 *
 * @param client A FluidIpcClient that is connected to a cybr instance
 * @param trackName See documentation for cybr.audiotrack.select()
 * @param plugin Plugin Object
 * @param n nth plugin
 * @param bpm Beats per minute
 *
 * @returns {ReaperVst}
 */
export async function vst2ToReaperObject(client: FluidIpcClient, trackName: string, plugin: FluidPlugin, n: number, bpm: number) {

  const cybrType = (plugin.pluginType === PluginType.unknown) ? undefined : plugin.pluginType;
  const pluginName = plugin.pluginName;
  
  // Get the normalized value of all 
  const paramSetters : any[] = [];
  for (const [paramKey, explicitValue] of Object.entries(plugin.parameters)) {
    const paramName = plugin.getParameterName(paramKey);
    if (typeof explicitValue === 'number') {
      const normalizedValue = plugin.getNormalizedValue(paramKey, explicitValue);
      if (typeof normalizedValue === 'number') {
        paramSetters.push(cybr.plugin.setParamNormalized(paramName, normalizedValue));
      } else {
        paramSetters.push(cybr.plugin.setParamExplicit(paramName, explicitValue));
      }
    } else {
      console.warn(`found non-number parameter value in ${plugin.pluginName} - ${paramKey}: ${explicitValue}`);
    }
  }

  const msg = [
    cybr.audiotrack.select(trackName),
    cybr.plugin.select(pluginName, cybrType, n),
    paramSetters,
    cybr.plugin.getReport(),
  ]
  const retObj = await client.send(msg);

  const pluginObject = JSON.parse(retObj.elements[3].args[2].value);
  const vst2State = pluginObject.vst2State;

  let isI   = pluginObject.isSynth ? 'i' : '';
  let name  = pluginObject.externalPluginFormat + isI + ': ' 
            + pluginObject.name + ' (' + pluginObject.vendor + ')';
  let id    = pluginObject.uidInt;

  let newVst = new rppp.objects.ReaperVst();
  newVst.params[0] = name;
  newVst.params[1] = pluginObject.name + '.' + pluginObject.pluginType;
  newVst.params[2] = 0;
  newVst.params[3] = "";
  newVst.params[4] = id + '<>';
  newVst.params[5] = "";
  newVst.params[6] = vst2State;
  newVst.params[7] = "";

  // Automation
  for (const [paramKey, automation] of Object.entries(plugin.automation)) {
    // Create an automation lane for each parameter with automation. Notice that
    // reaper identifies parameters by a numerical index, so we must retrieve
    // that index from the plugin adapter.
    const automationTrack     = new rppp.objects.ReaperPluginAutomation();
    automationTrack.params[0] = plugin.getParameterIndex(paramKey);
    newVst.add(automationTrack);

    // iterate over points. Ex { startTime: 0, value: 0.5, curve: 0 }
    for (const autoPoint of automation.points) {
      if (typeof autoPoint.value === 'number') {

        const explicitValue   = autoPoint.value;
        const normalizedValue = plugin.getNormalizedValue(paramKey, explicitValue);

        if (typeof normalizedValue === 'number') {
          automationTrack.addBezierPoint(
            autoPoint.startTime * 4 * 60 / bpm, 
            normalizedValue,
            autoPoint.curve
          );
        } else { 
          // If parameter does not have a normalized value, then ignore it.
          console.warn(`${pluginName}: no normalizer found for ${paramKey} param. Automation skipped.`);
        }
      }
    } // for (autoPoint of automation.points)
  }   // for (paramName, automation of plugin.automation)

  return newVst;
}