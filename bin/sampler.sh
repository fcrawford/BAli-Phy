#!/bin/sh
#$ -l serial -V -S /bin/sh -cwd

# A script for running bali-phy on the Sun Grid Engine (SGE)

#### CLASS=[class] VERSION=[version] sampler.sh align=data.fasta [optargs]

# CLASS is optional, but VERSION is required.

# We assume that binaries are in ~/bin/bali-phy/$VERSION

#---- Are we debugging? ----
if [ "$DEBUG" ] ; then
    DEBUG="-DEBUG";
else
    DEBUG="-NDEBUG";
fi

#---- find out which run class we are in ----
if [ "$CLASS" ]; then
    true;
else
    CLASS=normal
fi

#----- Determine the direction to run in -----
count=1
while [ -e $"$CLASS-$count" ] ; do
    (( count++ ))
done

DIR="$CLASS-$count"

#---- Make the directory and move into it ----
mkdir $DIR
cd $DIR

#------ Link in the data directory ------
if [ ! -e Data ] ; then
    ln -sf ~/Devel/Sampler/Data
fi

#------ start the sampler with the specified args ----
qsub -o cout -e cerr ~/bin/run.sh ~/bin/bali-phy/${VERSION}${DEBUG} "$@"