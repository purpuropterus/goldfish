#include "RPGolConfig.h"
#include "RPGolDefine.h"
#include "RPGolDifficulty.h"
#include "RPGolWindSet.h"
#include "RPUtlRandom.h"

/// <summary>
/// Generates a random sequence of numbers
/// between 0-max (exclusive), with no duplicates.
/// (This uses the original algorithm from Wii Sports.)
/// </summary>
/// <param name="max">Upper limit of sequence</param>
/// <param name="pArray">Pointer to array to fill</param>
void RPGolConfig::MakeRandomSequence(s32 max, s32 *pArray, bool ver_1_0)
{
    for (s32 i = 0; i < max; i++)
        pArray[i] = -max;

    s32 slotsFilled = 0;
    while (true)
    {
        s32 random = (s32)(RPUtlRandom::getF32(ver_1_0) * (max - slotsFilled));

        if (max > 0)
        {
            for (s32 i = 0, j = 0; i < max; i++)
            {
                if ((pArray[j] < 0) && (--random < 0))
                {
                    pArray[j] += max;
                    pArray[slotsFilled++] += j;

                    if (slotsFilled >= max)
                        return;

                    break;
                }
                else
                    j++;
            }
        }
    }
}

/// <summary>
/// Generate a pool of random speeds and directions, and choose 9 of each
/// to represent the "wind set" for the current game.
/// (This uses the original algorithm from Wii Sports.)
/// </summary>
void RPGolConfig::MakeWindSet(const RPGolDifficulty &diff, RPGolWindSet &out, bool ver_1_0)
{
    // Simulate all random number generations before wind is generated
    RPUtlRandom::advance(CALC_BEFORE_WIND);

    // Generate random sequences
    // (speed/dir, values this function will hand-pick from)
    s32 randomDirs[RPGolDefine::MAX_WIND_DIR];
    s32 randomSpeeds[RPGolDefine::MAX_WIND_SPD];
    MakeRandomSequence(RPGolDefine::MAX_WIND_DIR, randomDirs, ver_1_0);
    MakeRandomSequence(RPGolDefine::MAX_WIND_SPD, randomSpeeds, ver_1_0);

    // Round length (in holes)
    u32 numHoles = diff.endHole - diff.startHole;

    // Holes that have non-zero wind speed
    u32 numNonzeroWinds = 0;

    // Indexes for advancing through the random sequences
    u32 randomSpeedsIdx = 0;
    u32 randomDirsIdx = 0;

    // Loop through each hole
    for (u32 i = 0; i < RPGolDefine::HOLE_SIZE; i++)
    {
        // Will the hole NOT be played with the current difficulty?
        if ((i < diff.startHole) || (i > diff.endHole))
        {
            // If it won't, use dummy wind (32 mph S)
            // Setting the direction to 8 looks like a bug but it seems the game takes it modulo 8 later
            out[i].mDirection = RPGolDefine::MAX_WIND_DIR;
            out[i].mSpeed = RPGolDefine::MAX_WIND_SPD;
        }
        else
        {
            s32 nextSpd = 0;

            // Find the first random speed that is valid for the difficulty
            do
            {
                nextSpd = randomSpeeds[randomSpeedsIdx++];
            } while ((nextSpd < diff.minWind) || (nextSpd > diff.maxWind));

            out[i].mSpeed = nextSpd;

            // Only 8 out of the 9 holes can have non-zero wind speed
            if (numNonzeroWinds < 8)
            {
                // Assign random wind direction
                out[i].mDirection = randomDirs[randomDirsIdx];

                if (nextSpd <= 0)
                    continue;
                numNonzeroWinds++;
                randomDirsIdx++;
            }
            // This hole MUST have zero wind
            else
            {
                // If the next wind speed was going to be zero, we take it
                // We also set the wind direction to south
                if (nextSpd == 0)
                {
                    out[i].mDirection = RPGolDefine::SOUTH;
                    continue;
                }
                // Otherwise, we pick a random hole and override its speed with zero.
                else
                {
                    u32 rndHole = (u32)(RPUtlRandom::getF32(ver_1_0) * numHoles) + diff.startHole;
                    out[rndHole].mSpeed = 0;

                    // Copy wind direction from hole whose speed was set to zero
                    out[i].mDirection = out[rndHole].mDirection;
                }
            }
        }
    }
}