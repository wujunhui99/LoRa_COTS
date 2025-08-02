cd src/apps
project=$(ls)
cd $project
st-flash write $project'.bin' 0x8000000