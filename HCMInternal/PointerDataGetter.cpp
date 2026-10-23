#include "pch.h"
#include "PointerDataGetter.h"


namespace PointerDataGetter
{


    std::string getXMLDocument(std::string HCMdirPath)
    {
        constexpr std::string_view xmlFileName = "InternalPointerData.xml";



        auto pointerDataLocation = HCMdirPath + xmlFileName.data();

        // Halo Practice ships versioned pointer corrections alongside its binary.
        // Downloading upstream data here would silently replace those corrections.
        PLOG_INFO << "Using bundled Halo Practice pointer data (startup download disabled)";



        if (!fileExists(pointerDataLocation)) // no local copy? we're doomed, throw
        {
            throw HCMInitException(std::format("No pointer data existed at location {}", pointerDataLocation));
        }
        else
        {
            PLOG_VERBOSE << "Pointer Data getting contents of file at: " << pointerDataLocation;
        }

        return readFileContents(pointerDataLocation);

    }
}