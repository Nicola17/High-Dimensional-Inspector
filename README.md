[![Build Status](https://travis-ci.org/Nicola17/High-Dimensional-Inspector.svg?branch=master)](https://travis-ci.org/Nicola17/High-Dimensional-Inspector)

[![DOI](https://zenodo.org/badge/100361974.svg)](https://zenodo.org/badge/latestdoi/100361974)


# ![High Dimensional Inspector](./images/logo.png)
HDI is a library for the scalable analysis of large and high-dimensional data.
It contains scalable manifold-learning algorithms, visualizations and visual-analytics frameworks.
HDI is implemented in C++, OpenGL and JavaScript.
It is developed within a joint collaboration between the [Computer Graphics & Visualization](https://graphics.tudelft.nl/) group at the [Delft University of Technology](https://www.tudelft.nl) and the [Division of Image Processing (LKEB)](https://www.lumc.nl/org/radiologie/research/LKEB/) at the [Leiden Medical Center](https://www.lumc.nl/).

## Authors
- [Nicola Pezzotti](http://nicola17.github.io/) initiated the HDI project, developed the A-tSNE and HSNE algorithms and implemented most of the visualizations and frameworks.
- [Thomas Höllt](https://www.thomashollt.com/) ported the library to MacOS.

## Used
HDI is used in the following projects:
- [Cytosplore](https://www.cytosplore.org/): interactive system for understanding how the immune system works
- [Brainscope](http://www.brainscope.nl/brainscope): web portal for fast,
interactive visual exploration of the [Allen Atlases](http://www.brain-map.org/) of the adult and developing human brain
transcriptome
- [DeepEyes](https://graphics.tudelft.nl/Publications-new/2018/PHVLEV18/): progressive analytics system for designing deep neural networks

## Reference
Reference to cite when you use HDI in a research paper:

```
@inproceedings{Pezzotti2016HSNE,
  title={Hierarchical stochastic neighbor embedding},
  author={Pezzotti, Nicola and H{\"o}llt, Thomas and Lelieveldt, Boudewijn PF and Eisemann, Elmar and Vilanova, Anna},
  journal={Computer Graphics Forum},
  volume={35},
  number={3},
  pages={21--30},
  year={2016}
}
@article{Pezzotti2017AtSNE,
  title={Approximated and user steerable tsne for progressive visual analytics},
  author={Pezzotti, Nicola and Lelieveldt, Boudewijn PF and van der Maaten, Laurens and H{\"o}llt, Thomas and Eisemann, Elmar and Vilanova, Anna},
  journal={IEEE transactions on visualization and computer graphics},
  volume={23},
  number={7},
  pages={1739--1752},
  year={2017}
}
```

## Building
On Ubuntu 16.04 you can build and install HDI by running the following commands

```bash
apt-get install qtbase5-dev libqt5webkit5-dev libflann-dev
mkdir build
cd build
cmake  -DCMAKE_BUILD_TYPE=Release ..
make -j 8
sudo make install
```

## Testing
A test-driven-development framework is implemented using [Catch2](https://github.com/catchorg/Catch2).

To test the library you can run the test program in the tdd folder
```bash
./applications/tdd/tdd
```

Test for the visualization suit are located in the application/visual_tests folder. Here's a couple of applications that are worth checking:
```bash
./applications/visual_tests/tsne_line
./applications/visual_tests/data_viewers
./applications/visual_tests/linechart_view_test
```

## Approximated-tSNE (Without Progressive Visual Analytics)
You can run the Approximated-tSNE algorithm using the command line tool located
in ./applications/command_line_tools

Information on the arguments and options is available by calling the application with *-h*
```bash
./applications/command_line_tools/atsne_cmd -h
```

atsne_cmd requires 4 arguments:
- path/to/data: row-major orderer binary data (4Bytes floating point)
- path/to/output
- number of data points
- number of dimensions

You can test the A-tSNE application on a subset of the MNIST that is available in the *data* folder.

```bash
./applications/command_line_tools/atsne_cmd ../data/MNIST_1000.bin output.bin 1000 784
```

... and then check the output by using a simple viewer for the embedding.
```bash
./applications/command_line_tools/simple_embedding_viewer output.bin
```

With this two simple programs you must already have quite a good idea on how to
use HDI for dimensionality reduction and for visualizing the results.

## Approximated-tSNE (With Progressive Visual Analytics)
ToDo

## Hierarchical-SNE
ToDo
