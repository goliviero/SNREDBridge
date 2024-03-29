SNRedBridge package
===================

This SuperNEMO package will be used to trasnform SNFEE Raw Event Data (RED) into Falaise Unified Digitized Data (UDD)



Two programs are provided:

* ``red_bridge``:

  - reads a SNFEE RED file,
  - "converts" RED into UDD data for event header, calorimeter and tracker digitized hits
  - saves into a Falaise UDD file

* ``red_bridge_validation``:

  - reads a SNFEE RED file and a Falaise UDD file,
  - compares elements by elements each object of the two files and each data type to see if they are equivalent


The ``SNFrontEndElectronics_`` library must be installed and setup on your system.

.. _SNFrontEndElectronics: https://gitlab.in2p3.fr/SuperNEMO-DBD/SNFrontEndElectronics


#. Checks the SNFrontEndElectronics setup:

   .. code:: bash

      $ snfee_setup
      $ which snfee-query
      $ snfee-query --help
   ..

#. Build from the source directory:

   .. code:: bash

      $ cd tools
      $ ./build.bash
   ..

#. Run the ``red_bridge`` program:

   .. code:: bash

      $ cd ../install.d
      $ ./red_bridge --help
   ..

   .. code:: bash

      $ cd ../install.d
      $ ./red_bridge \
	     -i "/sps/nemo/snemo/snemo_data/raw_data/RED/snemo_run-815_red-v1.data.gz"
         -o "snemo_run-815_udd-v1.data.gz"
	     -n 1000

#. Run the ``red_bridge_validation`` program:

   .. code:: bash

      $ cd ../install.d
      $ ./red_bridge_validation --help
   ..

   .. code:: bash

      $ cd ../install.d
      $ ./red_bridge_validation \
	     -ired "/sps/nemo/snemo/snemo_data/raw_data/RED/snemo_run-815_red-v1.data.gz"
	     -iudd "snemo_run-815_udd-v1.data.gz"
	     -n 1000

.. end





#
