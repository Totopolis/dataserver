if [ $# -eq 0 ] 
then
   echo "No arguments supplied"
   export DEPLOY_BUILD_TYPE=Release
else
   export DEPLOY_BUILD_TYPE=$1
fi

echo DEPLOY_BUILD_TYPE=${DEPLOY_BUILD_TYPE}

. ./setup_env.sh

mkdir -p ${BUILD_DIR}
mkdir -p ${DEPLOY_DIR}

./dataserver_build.sh
