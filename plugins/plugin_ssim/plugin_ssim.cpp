#include <boost/filesystem.hpp>

#include "plugin_framework/plugin_interface.h"
#include "json.hpp"
#include "icon_analysis.h"
#include "lodepng.h"

using json = nlohmann::json;

namespace plugin
{

class HelloWorldPlugin : public IPlugin
{

    int get_api_version() const override { return 1; }

    pString get_id() const override {
        return boost::make_shared<std::string>("icon_analyze");
    }

    pString get_description() const override {
        return boost::make_shared<std::string>("Compare the PE icon with a database of icons (Need to put the icos in the database)");
    }

    pResult analyze(const mana::PE& pe) override
    {

        pResult res = create_result();

        mana::shared_resources resources = pe.get_resources();

        std::vector<Byte> buffer;

        for (mana::pResource r : *resources )
        {
            if (( *(r->get_type()) == "RT_GROUP_ICON") || ( *(r->get_type()) == "RT_GROUP_CURSOR")) {
                buffer = mana::reconstruct_icon(r->interpret_as<mana::pgroup_icon_directory>(), *resources);
            }
        }

        if (!buffer.empty())
        {
            // We need an IconDir to hold the data
            boost::shared_ptr<IconDir> p_icondir(new IconDir);      

            std::vector<DataIcon> database, pe_dataicon, db_dataicon;
            float result = 0, h_result = 0;
            Word info_dimension = 0, dimension_m, dimension_o, dimension_i;
            Long i = 0, nb_bytes_read = 0, l;
            std::string fname, info_name;
            std::string p_ico, p_json;
            json jo; 

            ico_header_read(p_icondir, buffer, nb_bytes_read);
            pe_dataicon = ico_entries_read(p_icondir, buffer, "pe", nb_bytes_read);
            std::sort(pe_dataicon.begin(), pe_dataicon.end(), ico_sort);

            buffer.clear();

            // Creation of the database
            for (boost::filesystem::directory_entry & f : boost::filesystem::directory_iterator("./../plugins/plugin_ssim/dataicon/ico/"))
            {
                // Check if it is an ico
                if (f.path().leaf().extension().string() == ".ico")
                {
                    fname = f.path().leaf().string();
                    fname = fname.substr(0, fname.size() - 4);

                    file_to_buf(f.path().string(), buffer);

                    if (!buffer.empty())
                    {
                        nb_bytes_read = 0;
                        p_icondir = boost::shared_ptr<IconDir>(new IconDir);
                        ico_header_read(p_icondir, buffer, nb_bytes_read);
                        db_dataicon = ico_entries_read(p_icondir, buffer, fname, nb_bytes_read);

                        for (DataIcon io : db_dataicon) {
                            database.push_back(io);
                        }

                        buffer.clear();
                    }
                }   
            }

/*          
            //Version with json save

            for (boost::filesystem::directory_entry & f : boost::filesystem::directory_iterator("./../plugins/plugin_ssim/dataicon/ico/"))      // Creation of the database
            {
                if (f.path().leaf().extension().string() == ".ico")
                {
                    fname = f.path().leaf().string();
                    fname = fname.substr(0, fname.size() - 4);
                    p_json = "./../plugins/plugin_ssim/dataicon/json/" + fname + ".json";

                    // The Json file is already in the database
                    if (boost::filesystem::exists(p_json))  
                    {
                        std::ifstream iput(p_json);
                        iput >> jo; 
                        iput.close();

                        for (auto & el : jo.items())
                        {
                            if (el.value() != nullptr)
                            {
                                auto name = el.value().at("name").get<std::string>();
                                auto gray_table = el.value().at("gray_table").get<std::vector<Byte>>();
                                auto dimension = (Word) std::stoi(el.key()) * MIN_ICON_SIZE;
                                auto mean = el.value().at("mean").get<float>();
                                auto variance = el.value().at("variance").get<float>();

                                DataIcon io(name, gray_table, dimension, mean, variance);

                                database.push_back(io);
                            }
                        }   
                    }

                    // the Json file is not in the database, so we create it
                    else                                    
                    {
                        p_ico = f.path().string();

                        file_to_buf(p_ico, buffer);

                        if (!buffer.empty())
                        {
                            nb_bytes_read = 0;
                            p_icondir = boost::shared_ptr<IconDir>(new IconDir);
                            ico_header_read(p_icondir, buffer, nb_bytes_read);
                            db_dataicon = ico_entries_read(p_icondir, buffer, fname, nb_bytes_read);

                            buffer.clear();
                            
                            for (DataIcon io : db_dataicon)
                            {
                                database.push_back(io);
                                dimension_o = io.get_dimension() / MIN_ICON_SIZE;
                                jo[dimension_o]["name"] = fname;
                                jo[dimension_o]["gray_table"] = *(io.get_gray_table());
                                jo[dimension_o]["mean"] = io.get_mean();
                                jo[dimension_o]["variance"] = io.get_variance();
                            }

                            std::ofstream oput(p_json);
                            oput << jo << std::endl;
                            oput.close();
                        }
                    }
                }
            }
*/

            if (!database.empty())
            {
                std::sort(database.begin(), database.end(), ico_sort);
                l = database.size();

                // Comparison with the database
                for (DataIcon im : pe_dataicon)                 
                {
                    dimension_i = database[i].get_dimension();
                    dimension_m = im.get_dimension();
                    while (i < l && dimension_i <= dimension_m)
                    {
                        if (dimension_i == dimension_m)
                        {
                            result = im.ssim(database[i]);
                            if (h_result <= result)
                            {
                                h_result = result;
                                info_name = database[i].get_name();
                                info_dimension = dimension_m;
                            }
                        }
                        ++i;
                        dimension_i = database[i].get_dimension();
                    }
                }

                // Result
                res->set_level(SUSPICIOUS);

                if (h_result == 1) {
                    res->set_summary("Match");
                }

                else if (h_result > 0.95) {
                    res->set_summary("Highly similar");
                }

                else if (h_result > 0.75) {
                    res->set_summary("Quite similar");
                }

                else if (h_result > 0.4) {
                    res->set_summary("Little similar");
                }

                else {
                    res->set_summary("Not similar");
                }

                res->add_information("Name ", info_name);
                res->add_information("Icon size ", std::to_string(info_dimension) + "x" + std::to_string(info_dimension));
                res->add_information("Similarity ", h_result);
            }

            else
            {
                res->set_level(NO_OPINION);
                res->set_summary("No ico found in the database\n Please put the icos in the following file :");
                res->add_information("Manalyze/plugins/plugin_ssim/dataicon/ico");
            }
        }

        else
        {
            res->set_level(NO_OPINION);
            res->set_summary("Problem with the ico of the PE");
            res->add_information("Ico can't be read");
        }

        return res;
    }
};

// ----------------------------------------------------------------------------

extern "C"
{
    PLUGIN_API IPlugin* create() { return new HelloWorldPlugin(); }
    PLUGIN_API void destroy(IPlugin* p) { delete p; }
};

} /* !namespace plugin */