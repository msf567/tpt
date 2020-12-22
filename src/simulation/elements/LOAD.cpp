#include "simulation/ElementCommon.h"

#include "music/audio.h"
#include "music/music.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_LOAD()
{
    //Identifier, used for lua and internal stuff, change HETR to your element's name
    Identifier = "DEFAULT_PT_LOAD";

    //Name, it is recommended to use 4 letters, but less works. 5 or more will probably not fit on the buttons
    Name = "LOAD";

    //Default color
    Colour = PIXPACK(0xFFFF00);

    //If it's visible in the menu, if 0 then it is hidden and can only be produced with reactions or lua
    MenuVisible = 1;

    //The menu section it's in, see src/simulation/SimulationData.h for a list, but you can probably guess their names on your own
    MenuSection = SC_SPECIAL;

    //If 0, it cannot be created or seen in any way, not even lua. It will just disappear if somehow created. Used for removed elements, you should leave this 1
    Enabled = 1;

    //How much the particle is accelerated by moving air. Normally 0 for solids, and up to 1 for other elements. It can be negative, ANAR and DEST do this so it goes towards pressure
    Advection = 0.0f;

    //How much air the particle generates in the direction of travel. Generally is very small, 0.04f creates a lot of positive air (- creates negative pressure).
    AirDrag = 0.00f * CFDS;

    //How much the particle slows down moving air (although not as big an effect as a wall). 1 = no effect, 0 = maximum effect. Solids are generally above 0.90f, along with most other elements too
    AirLoss = 0.90f;

    //How much velocity the particle loses each frame. 1 = no loss, .5 = half loss. Solids have it at 0. Only a few have it at 1, like energy particles, and old moving sponge.
    Loss = 0.00f;

    //Velocity is multiplied by this when the particle collides with something. Energy particles have it at -0.99f, everything else is -0.01f or 0.0f. This property does not do much at all.
    Collision = 0.0f;

    //How fast the particle falls. A negative number means it floats. Generally very small, most gasses are negative, everything else is usually less than 0.04f
    Gravity = 0.0f;

    //How much the particle "wiggles" around (think GAS or HYGN). Set at 0, except for gasses, which is a positive number. Up to 3 (or higher) for a large amount of wiggle, GAS is 0.75f, HYGN is 3.00f
    Diffusion = 0.00f;

    //How much the particle increases the pressure by. Another property only for gasses, but VENT/VACU have theirs at (-)0.010f. An extremely small number, sometimes as small as 0.000001f
    HotAir = 0.000f * CFDS;

    //How does the particle move? 0 = solid, gas, or energy particle, 1 = powder, 2 = liquid.
    Falldown = 0;

    //Does it burn? 0 = no, higher numbers = higher "burnage". Something like 20 is WOOD, while C-4 is 1000. Some are a few thousand for almost instant burning.
    Flammable = 0;

    //Does it explode? 0 = no, 1 = when touching fire, 2 = when touching fire or when pressure > 2.5. Yes, those are the only options, see FIRE.cpp or somewhere in Simulation.cpp to modify how they work
    Explosive = 0;

    //Does it melt? 1 or higher = yes, 0 = no. This is actually only used in legacy mode, not paying attention to this property once caused a bug where many solids that don't melt would without heat sim on.
    Meltable = 0;

    Hardness = 0; //How much does acid affect it? 0 = no effect, higher numbers = higher effect. Generally goes up to about 50.

    Weight = 100; //Heavier elements sink beneath lighter ones. 1 = Gas. 2 = Light, 98 = Heavy (liquids 0-49, powder 50-99). 100 = Solid. -1 is Neutrons and Photons.

    //0 - no het transfer, 255 - maximum heat transfer speed.
    HeatConduct = 251;

    //A short one sentence description of the element, shown when you mouse over it in-game.
    Description = "Loads the next level";

    //Does this element have special properties? Properties are listed in src/simulation/Element.h, you at least need to have the correct state property. If you want it to conduct electricity, be sure to use both PROP_CONDUCTS and PROP_LIFE_DEC
    Properties = TYPE_SOLID | PROP_LIFE_DEC;

    LowPressure = IPL;
    LowPressureTransition = NT;
    HighPressure = IPH;
    HighPressureTransition = NT;
    LowTemperature = ITL;
    LowTemperatureTransition = NT;
    HighTemperature = ITH;
    HighTemperatureTransition = NT;

    Update = &update;
    Graphics = &graphics;
}

//#TPT-Directive ElementHeader Element_HETR static int update(UPDATE_FUNC_ARGS)
static int update(UPDATE_FUNC_ARGS)
{
    int targetParticle, searchX, searchY;
    float freq = NOTE::get_frequency_from_key(parts[i].tmp);
    if (freq < 0)
        freq = 440.0f;
    for (searchX = -1; searchX <= 1; searchX++)
        for (searchY = -1; searchY <= 1; searchY++)
            if (BOUNDS_CHECK && (searchX || searchY))
            {
                targetParticle = pmap[y + searchY][x + searchX];
                if (!targetParticle)
                    continue;

                if (TYP(targetParticle) == PT_SPRK)
                {
                    sim->LoadNextSave();
                }
            }
    return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
    // graphics code here
    // return 1 if nothing dymanic happens here

    return 1;
}
