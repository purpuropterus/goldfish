#include "server/src/core/finders/finder_bridge.h"

RPGolWindSet inputFFIToWindSet(const OgWindFinderInputFFI &input, const size_t size = RPGolDefine::HOLE_SIZE)
{
    RPGolWindSet windSet(size);

    // std::cout << windSet.mSize << std::endl;

    for (size_t i = 0; i < windSet.mSize; i++)
    {
        windSet.mWinds[i].mDirection = input.winds[i].mDirection;
        windSet.mWinds[i].mSpeed = input.winds[i].mSpeed;
    }
    return windSet;
}

OgWindFinderOutputFFI windSetToOutputFFI(const RPGolWindSet &windSet)
{
    OgWindFinderOutputFFI output;

    for (u32 i = 0; i < windSet.mSize; i++)
    {
        output.winds[i].mDirection = windSet.mWinds[i].mDirection;
        output.winds[i].mSpeed = windSet.mWinds[i].mSpeed;
    }

    return output;
}

OgWindFinderOutputWithErrorFFI find_og_wind(const OgWindFinderInputFFI &input, const OgWindFinderSettings &settings)
{
    OgWindFinderOutputWithErrorFFI finalOutput;

    try
    {

        bool ver_1_0 = settings.game == 0;

        const char *precomputeEnvVarName;
        std::string precomputeDefaultPath;

        switch (settings.game)
        {
        case 0:
            precomputeEnvVarName = "OG_WIND_PRECOMPUTE_PATH_1_0";
            precomputeDefaultPath = "/og_wind_precompute_1.0.bin";
            break;
        case 1:
            precomputeEnvVarName = "OG_WIND_PRECOMPUTE_PATH_1_1";
            precomputeDefaultPath = "/og_wind_precompute_1.1.bin";
            break;
        case 2:
            precomputeEnvVarName = "WSR_WIND_PRECOMPUTE_PATH";
            precomputeDefaultPath = "/wsr_wind_precompute.bin";
            break;
        default:
            throw std::runtime_error("Unknown game");
        }

        const char *env = std::getenv(precomputeEnvVarName);

        const std::string filePath =
            (env && env[0] != '\0')
                ? std::string(env)
                : precomputeDefaultPath;

        // find outputs based on game number

        std::vector<OgWindFinderOutput> outputs;

        if (settings.game == 2)
        {
            WsrWindFinder finder(filePath);
            RPGolWindSet windSet = inputFFIToWindSet(input, 21);

            // print windset
            char buffer[1024];
            windSet.toString(buffer);
            std::cout << buffer << std::endl;

            outputs = finder.find(windSet, settings.last_known_seed, settings.num_to_check);
        }
        else if (settings.game == 0 || settings.game == 1)
        {
            OgWindFinder finder(filePath, ver_1_0);
            RPGolWindSet windSet = inputFFIToWindSet(input, 9);
            outputs = finder.find(windSet, settings.last_known_seed, settings.num_to_check);
        }

        // std::cout << outputs.size() << std::endl;

        for (const auto &output : outputs)
        {
            OgWindFinderOutputFFI outputFFI = windSetToOutputFFI(output.windSet);
            outputFFI.seed = output.seed;
            finalOutput.seeds.push_back(outputFFI);
        }
    }
    catch (const std::exception &e)
    {
        std::cout << rust::String(e.what()) << std::endl;
        finalOutput.error = rust::String("An error occured");
    }
    return finalOutput;
}

BlinkGroup inputFFIToBlinkGroup(const OgBlinkFinderInputFFI &input)
{
    return BlinkGroup(std::vector<u32>(input.blinks.begin(), input.blinks.end()));
}

ScoredOgBlinkFinderOutputFFI blinkOutputToOutputFFI(const ScoredOutput<OgBlinkFinderOutput> &output)
{
    ScoredOgBlinkFinderOutputFFI outputFFI;
    outputFFI.score = output.score;
    outputFFI.output.seed = output.output.seed;

    for (u32 blink : output.output.blinkGroup.blinkTimes)
    {
        outputFFI.output.blinks.push_back(blink);
    }

    return outputFFI;
}

ScoredOgBlinkFinderOutputWithErrorFFI find_og_blink(const OgBlinkFinderInputFFI &input, const OgBlinkFinderSettings &settings)
{
    ScoredOgBlinkFinderOutputWithErrorFFI finalOutput;

    try
    {
        const char *precomputeEnvVarName = "OG_BLINK_PRECOMPUTE_PATH";
        const char *env = std::getenv(precomputeEnvVarName);
        const std::string filePath =
            (env && env[0] != '\0')
                ? std::string(env)
                : "/og_blink_precompute.bin";

        OgBlinkFinder finder(filePath);
        BlinkGroup blinkGroup = inputFFIToBlinkGroup(input);

        auto outputs = finder.find(blinkGroup, settings.last_known_seed, settings.num_to_check);

        for (const auto &output : outputs)
        {
            finalOutput.outputs.push_back(blinkOutputToOutputFFI(output));
        }
    }
    catch (const std::exception &e)
    {
        std::cout << rust::String(e.what()) << std::endl;
        finalOutput.error = rust::String("An error occured");
        // finalOutput.error = rust::String(e.what());
    }

    return finalOutput;
}