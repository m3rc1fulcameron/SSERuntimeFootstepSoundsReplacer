# Runtime Footstep Sound Replacer
This repository contains a SkyrimAE SKSE plugin based on CommonLibSSE for dynamically replacing footstep sounds to be played at runtime based off of TOML configuration files. This distribution contains one such configuration file for dynamically applying the footstep set from [Heels Sound](https://www.nexusmods.com/skyrimspecialedition/mods/62502) to all female actors wearing shoes/boots which have the "ClothingHH" keyword attached to them. This plugin exists because Skyrim does not contain functionality to dynamically change footstep sets without changing them globally for a specific ArmorAddon. The configuration file contained within this distribution fixes a specific problem: armor mods which contain both male and female variants for boots were previously restricted to using a single footstep set. This was awkward when the female model contained armor with heels.

## Requirements
* [Address Library](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
* [Heels Sound](https://www.nexusmods.com/skyrimspecialedition/mods/62502) (For the bundled config to work)

## Features
Currently, the plugin provides a basic rules engine for footstep events. Right now, it only supports the following predicates:
* actor.isFemale: Is the calling actor a female?
* actor.hasKeyword: Does the calling actor have a keyword?
* armor.\<slot>.hasKeyword: Does the armor equipped in a specific slot have a keyword?
* AND: Evaluates to true if all child predicates evaluate to true
* OR: Evaluates to true if one of the child predicates evaluate to true

In terms of future development, more predicates are rather trivial to add. At runtime, the function I hook has access to the calling Actor object, the original FootstepSet which was going to be used, and any global state.

## Future Development
There are a couple ideas I have for this: one could be replacing footstep sounds for characters wearing heavy armor vs those in lighter armor. Another could be changing the sounds played when a character is walking during a weather event (e.g. wet sounds when it's raining, dry when it's not).

## How does it work?
This plugin hooks a function called by the event handler for processing FootstepEvents at `0x1405d5b33` within the 1.6.640.0 binary. The function seems to determine which Footstep sound to play from the footstep set belonging to the ArmorAddon associated with whatever armor piece is equipped in the foot slot. Before calling the original function, this plugin runs the event through the rules engine to determine which FootstepSet to replace the original with. Once it's determined, it calls the original function.