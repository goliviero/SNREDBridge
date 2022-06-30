// Standard library:
#include <cstdio>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <memory>
#include <string>
#include <vector>

// Third party:
// - Bayeux:
#include <bayeux/datatools/logger.h>
#include <bayeux/datatools/io_factory.h>
// - Boost:
#include <boost/program_options.hpp>
namespace bpo = boost::program_options;

// - Falaise:
#include <falaise/snemo/datamodels/unified_digitized_data.h>
#include <falaise/snemo/datamodels/event_header.h>

// - SNFEE:
#include <snfee/snfee.h>
#include <snfee/io/multifile_data_reader.h>
#include <snfee/data/raw_event_data.h>


void do_red_to_udd_conversion(const snfee::data::raw_event_data,
                              snemo::datamodel::unified_digitized_data);

//----------------------------------------------------------------------
// MAIN PROGRAM
//----------------------------------------------------------------------

int main (int argc, char *argv[])
{
  std::string input_filename = "";
  std::string output_filename = "";

  for (int iarg=1; iarg<argc; ++iarg)
    {
      std::string arg (argv[iarg]);
      if (arg[0] == '-')
        {
          if (arg=="-i" || arg=="--input")
            input_filename = std::string(argv[++iarg]);

          else if (arg=="-o" || arg=="--output")
            output_filename = std::string(argv[++iarg]);

          else if (arg=="-h" || arg=="--help")
            {
              std::cout << std::endl;
              std::cout << "Usage:   " << argv[0] << " [options]" << std::endl;
              std::cout << std::endl;
              std::cout << "Options:   -h / --help" << std::endl;
              std::cout << "           -i / --input   RED_FILE" << std::endl;
              std::cout << "           -o / --output  UDD_FILE" << std::endl;
              std::cout << std::endl;
              return 0;
            }

          else
            std::cerr << "*** ERROR: unkown option " << arg << std::endl;
        }
    }

  if (input_filename.empty())
    {
      std::cerr << "*** ERROR: missing input filename !" << std::endl;
      return 1;
    }

  snfee::initialize();

  /// Configuration for raw data reader
  snfee::io::multifile_data_reader::config_type reader_cfg;
  reader_cfg.filenames.push_back(input_filename);

  // Instantiate a reader
  snfee::io::multifile_data_reader red_source(reader_cfg);

  // RED counter
  std::size_t red_counter = 0;

  while (red_source.has_record_tag())
    {
      // Check the serialization tag of the next record:
      DT_THROW_IF(!red_source.record_tag_is(snfee::data::raw_event_data::SERIAL_TAG),
                  std::logic_error, "Unexpected record tag '" << red_source.get_record_tag() << "'!");

      // Empty working RED object
      snfee::data::raw_event_data red;

      // Empty working UDD object
      snemo::datamodel::unified_digitized_data udd;

      // Load the next RED object:
      red_source.load(red);

      // Do the RED to UDD conversion
      do_red_to_udd_conversion(red, udd);

      std::clog << "DEBUG main : Exit do_red_to_udd_conversion" << std::endl;

      // Write UDD event in the output file
      // datatools::data_writer writer(output_filename,
      //                               datatools::using_multiple_archives);

      // Increment the counter
      red_counter++;

    } // (while red_source.has_record_tag())


  // Check input RED file and output UDD file and count the number of events in each file

  // UDD counter
  std::size_t udd_events = 0;

  // Read UDD file here


  std::cout << "Results :" << std::endl;
  std::cout << "- Worker #0 (input RED)" << std::endl;
  std::cout << "  - Processed records : " << red_counter << std::endl;
  std::cout << "- Worker #1 (output UDD)" << std::endl;
  std::cout << "  - Stored records : " << udd_events << std::endl;

  snfee::terminate();

  return 0;
}



void do_red_to_udd_conversion(const snfee::data::raw_event_data red_,
                              snemo::datamodel::unified_digitized_data udd_)
{
  // Run number
  int32_t red_run_id   = red_.get_run_id();

  // Event number
  int32_t red_event_id = red_.get_event_id();

  // Container of merged TriggerID(s) by event builder
  const std::set<int32_t> & red_trigger_ids = red_.get_origin_trigger_ids();

  // RED Digitized calo hits
  const std::vector<snfee::data::calo_digitized_hit> red_calo_hits = red_.get_calo_hits();

  // RED Digitized tracker hits
  const std::vector<snfee::data::tracker_digitized_hit> red_tracker_hits = red_.get_tracker_hits();

  // Print RED infos
  std::cout << "Event #" << red_event_id << " contains "
            << red_trigger_ids.size() << " TriggerID(s) with "
            << red_calo_hits.size() << " calo hit(s) and "
            << red_tracker_hits.size() << " tracker hit(s)"
            << std::endl;

  // Fill Event Header based on RED attributes
  snemo::datamodel::event_header EH;
  EH.get_id().set_run_number(red_run_id);
  EH.get_id().set_event_number(red_event_id);
  EH.set_generation(snemo::datamodel::event_header::GENERATION_REAL);

  // GO: we have to decide how we handle time and timestamp at Falaise level, commenting for now...
  // EH.get_timestamp().set_seconds(1268644034);
  // EH.get_timestamp().set_picoseconds(666);

  // GO: we can add some additional properties to the Event Header
  // EH.get_properties().store("simulation.bundle", "falaise");
  // EH.get_properties().store("simulation.version", "0.1");
  // EH.get_properties().store("author", std::string(getenv("USER")));


  // Copy RED attributes to UDD attributes
  udd_.set_run_id(red_run_id);
  udd_.set_event_id(red_event_id);
  udd_.set_reference_timestamp(red_.get_reference_time().get_ticks());
  udd_.set_origin_trigger_ids(red_trigger_ids);
  udd_.set_auxiliaries(red_.get_auxiliaries());

  // Scan and copy RED calo digitized hit into UDD calo digitized hit:
  for (std::size_t ihit = 0; ihit < red_calo_hits.size(); ihit++)
    {
      std::clog << "DEBUG do_red_to_udd_conversion : Calo hit #" << ihit << std::endl;
      snfee::data::calo_digitized_hit red_calo_hit = red_calo_hits[ihit];
      snemo::datamodel::calorimeter_digitized_hit & udd_calo_hit = udd_.add_calorimeter_hit();

      udd_calo_hit.set_geom_id(red_calo_hit.get_geom_id());
      udd_calo_hit.set_timestamp(red_calo_hit.get_reference_time().get_ticks());
      std::vector<int16_t> calo_waveform = red_calo_hit.get_waveform();
      udd_calo_hit.set_waveform(calo_waveform);
      udd_calo_hit.set_low_threshold_only(red_calo_hit.is_low_threshold_only());
      udd_calo_hit.set_high_threshold(red_calo_hit.is_high_threshold());
      udd_calo_hit.set_fcr(red_calo_hit.get_fcr());
      udd_calo_hit.set_lt_trigger_counter(red_calo_hit.get_lt_trigger_counter());
      udd_calo_hit.set_lt_time_counter(red_calo_hit.get_lt_time_counter());
      udd_calo_hit.set_fwmeas_baseline(red_calo_hit.get_fwmeas_baseline());
      udd_calo_hit.set_fwmeas_peak_amplitude(red_calo_hit.get_fwmeas_peak_amplitude());
      udd_calo_hit.set_fwmeas_peak_cell(red_calo_hit.get_fwmeas_peak_cell());
      udd_calo_hit.set_fwmeas_charge(red_calo_hit.get_fwmeas_charge());
      udd_calo_hit.set_fwmeas_rising_cell(red_calo_hit.get_fwmeas_rising_cell());
      udd_calo_hit.set_fwmeas_falling_cell(red_calo_hit.get_fwmeas_falling_cell());
      snemo::datamodel::calorimeter_digitized_hit::rtd_origin the_rtd_origin(red_calo_hit.get_origin().get_hit_number(),
                                                                             red_calo_hit.get_origin().get_trigger_id());
      udd_calo_hit.set_origin(the_rtd_origin);

    } // end of for ihit



  // Scan and copy RED tracker digitized hit into UDD calo digitized hit:
  for (std::size_t ihit = 0; ihit < red_tracker_hits.size(); ihit++)
    {
      std::clog << "DEBUG do_red_to_udd_conversion : Tracker hit #" << ihit << std::endl;
      snfee::data::tracker_digitized_hit red_tracker_hit = red_tracker_hits[ihit];
      snemo::datamodel::tracker_digitized_hit & udd_tracker_hit = udd_.add_tracker_hit();
      udd_tracker_hit.set_geom_id(red_tracker_hit.get_geom_id());

      // Do the loop on RED GG timestamps and convert them into UDD GG timestamps
	  const std::vector<snfee::data::tracker_digitized_hit::gg_times> gg_timestamps_v = red_tracker_hit.get_times();

      for (std::size_t iggtime = 0; iggtime < gg_timestamps_v.size(); iggtime++)
        {
          std::clog << "DEBUG do_red_to_udd_conversion : Tracker hit #" << ihit << " Geiger Time #" << iggtime <<  std::endl;
          // Retrieve RED GG timestamps
	      const snfee::data::tracker_digitized_hit::gg_times & a_gg_timestamp = gg_timestamps_v[iggtime];

          // Create empty UDD GG timestamps
          snemo::datamodel::tracker_digitized_hit::gg_times udd_gg_timestamp;

          // Fill UDD anode and cathode timestamps and RTD origin for backtracing
          snemo::datamodel::tracker_digitized_hit::rtd_origin anode_r0_rtd_origin(a_gg_timestamp.get_anode_origin(snfee::data::tracker_digitized_hit::ANODE_R0).get_hit_number(),
                                                                                  a_gg_timestamp.get_anode_origin(snfee::data::tracker_digitized_hit::ANODE_R0).get_trigger_id());
          udd_gg_timestamp.set_anode_origin(snemo::datamodel::tracker_digitized_hit::ANODE_R0,
                                            anode_r0_rtd_origin);
          udd_gg_timestamp.set_anode_time(snemo::datamodel::tracker_digitized_hit::ANODE_R0,
                                          a_gg_timestamp.get_anode_time(snfee::data::tracker_digitized_hit::ANODE_R0).get_ticks());

          snemo::datamodel::tracker_digitized_hit::rtd_origin anode_r1_rtd_origin(a_gg_timestamp.get_anode_origin(snfee::data::tracker_digitized_hit::ANODE_R1).get_hit_number(),
                                                                                  a_gg_timestamp.get_anode_origin(snfee::data::tracker_digitized_hit::ANODE_R1).get_trigger_id());
          udd_gg_timestamp.set_anode_origin(snemo::datamodel::tracker_digitized_hit::ANODE_R1,
                                            anode_r1_rtd_origin);
          udd_gg_timestamp.set_anode_time(snemo::datamodel::tracker_digitized_hit::ANODE_R1,
                                          a_gg_timestamp.get_anode_time(snfee::data::tracker_digitized_hit::ANODE_R1).get_ticks());

          snemo::datamodel::tracker_digitized_hit::rtd_origin anode_r2_rtd_origin(a_gg_timestamp.get_anode_origin(snfee::data::tracker_digitized_hit::ANODE_R2).get_hit_number(),
                                                                                  a_gg_timestamp.get_anode_origin(snfee::data::tracker_digitized_hit::ANODE_R2).get_trigger_id());
          udd_gg_timestamp.set_anode_origin(snemo::datamodel::tracker_digitized_hit::ANODE_R2,
                                            anode_r2_rtd_origin);
          udd_gg_timestamp.set_anode_time(snemo::datamodel::tracker_digitized_hit::ANODE_R2,
                                          a_gg_timestamp.get_anode_time(snfee::data::tracker_digitized_hit::ANODE_R2).get_ticks());

          snemo::datamodel::tracker_digitized_hit::rtd_origin anode_r3_rtd_origin(a_gg_timestamp.get_anode_origin(snfee::data::tracker_digitized_hit::ANODE_R3).get_hit_number(),
                                                                                  a_gg_timestamp.get_anode_origin(snfee::data::tracker_digitized_hit::ANODE_R3).get_trigger_id());
          udd_gg_timestamp.set_anode_origin(snemo::datamodel::tracker_digitized_hit::ANODE_R3,
                                            anode_r3_rtd_origin);
          udd_gg_timestamp.set_anode_time(snemo::datamodel::tracker_digitized_hit::ANODE_R3,
                                          a_gg_timestamp.get_anode_time(snfee::data::tracker_digitized_hit::ANODE_R3).get_ticks());

          snemo::datamodel::tracker_digitized_hit::rtd_origin anode_r4_rtd_origin(a_gg_timestamp.get_anode_origin(snfee::data::tracker_digitized_hit::ANODE_R4).get_hit_number(),
                                                                                  a_gg_timestamp.get_anode_origin(snfee::data::tracker_digitized_hit::ANODE_R4).get_trigger_id());
          udd_gg_timestamp.set_anode_origin(snemo::datamodel::tracker_digitized_hit::ANODE_R4,
                                            anode_r4_rtd_origin);
          udd_gg_timestamp.set_anode_time(snemo::datamodel::tracker_digitized_hit::ANODE_R4,
                                          a_gg_timestamp.get_anode_time(snfee::data::tracker_digitized_hit::ANODE_R4).get_ticks());

          snemo::datamodel::tracker_digitized_hit::rtd_origin bottom_cathode_rtd_origin(a_gg_timestamp.get_bottom_cathode_origin().get_hit_number(),
                                                                                        a_gg_timestamp.get_bottom_cathode_origin().get_trigger_id());
          udd_gg_timestamp.set_bottom_cathode_origin(bottom_cathode_rtd_origin);
          udd_gg_timestamp.set_bottom_cathode_time(a_gg_timestamp.get_bottom_cathode_time().get_ticks());


          snemo::datamodel::tracker_digitized_hit::rtd_origin top_cathode_rtd_origin(a_gg_timestamp.get_top_cathode_origin().get_hit_number(),
                                                                                     a_gg_timestamp.get_top_cathode_origin().get_trigger_id());
          udd_gg_timestamp.set_top_cathode_origin(top_cathode_rtd_origin);
          udd_gg_timestamp.set_top_cathode_time(a_gg_timestamp.get_top_cathode_time().get_ticks());


	    } // end of iggtime

    } // end for ihit


  return;
}
