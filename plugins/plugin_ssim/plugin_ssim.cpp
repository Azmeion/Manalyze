#include "/home/shadows/Documents/Manalyze/Manalyze/include/plugin_framework/plugin_interface.h"
#include "icon_analysis.h"
#include "lodepng.h"
#include <fstream>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <math.h>
#include <iostream>

namespace plugin
{

class HelloWorldPlugin : public IPlugin
{

    int get_api_version() const override { return 1; }

    pString get_id() const override {
        return boost::make_shared<std::string>("icon_analyze");
    }

    pString get_description() const override {
        return boost::make_shared<std::string>("compare the PE icon with a database of icons");
    }

    pResult analyze(const mana::PE& pe) override
    {
        pResult res = create_result();

        std::vector<std::string> info_en_plus;

        mana::shared_resources resources = pe.get_resources();

        std::vector<BYTE> buffer;

        for (mana::pResource r : *resources ) {
            if (( *(r->get_type()) == "RT_GROUP_ICON") || ( *(r->get_type()) == "RT_GROUP_CURSOR")) {

            buffer = mana::reconstruct_icon(r->interpret_as<mana::pgroup_icon_directory>(), *resources);

            }
        }

        // We need an ICONDIR to hold the data
        ICONDIR * pIconDir = new ICONDIR; // TODO : Utiliser des jolis pointeurs.
        ICONDIR * pIconDir2 = new ICONDIR; // TODO : Utiliser des jolis pointeurs.
        std::vector<BYTE> buffer2;
        //fileToBuf("/home/shadows/Documents/Manalyze/Manalyze/plugins/plugin_ssim/dataicon/app.ico", buffer);
        fileToBuf("/home/shadows/Documents/Manalyze/Manalyze/plugins/plugin_ssim/dataicon/office.ico", buffer2); //Path à configurer
        dataIcon math, office;
        float result;

        if (!buffer.empty())
        {
            int nbBytesRead = 0;
            icoHeaderRead(pIconDir, buffer, nbBytesRead);
            math = icoEntriesRead(pIconDir, buffer, nbBytesRead);
            delete pIconDir;
            buffer.clear();
        }
        if (!buffer2.empty())
        {
            int nbBytesRead = 0;
            icoHeaderRead(pIconDir2, buffer2, nbBytesRead);
            office = icoEntriesRead(pIconDir2, buffer2, nbBytesRead);
            delete pIconDir2;
            buffer2.clear();
        }

        if(math.getMean() && office.getMean())
        {
            result = office.ssim(math);
        }

        res->set_level(SUSPICIOUS);
        res->set_summary("Similarity with an icon");

        std::string test_visu = "Discord : " + std::to_string(result);

        info_en_plus.push_back(test_visu);

        res->add_information("Highest similarity ", result);
        res->add_information("Icon size ", "128x128");
        res->add_information("Similar icons ", info_en_plus);
/**/
        return res;
    }
};

// ----------------------------------------------------------------------------

extern "C"
{
    PLUGIN_API IPlugin* create() { return new HelloWorldPlugin(); }
    PLUGIN_API void destroy(IPlugin* p) { delete p; }
};

} //!namespace plugin