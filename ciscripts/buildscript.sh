#!/bin/bash
set -e

#BRANCH_NAME=${BRANCH_NAME:-master}
BRANCH_NAME="master"

export IMAGE_NAME="scorum/blockchain:$BRANCH_NAME"
if [[ $IMAGE_NAME == "scorum/blockchain:stable" ]] ; then
	IMAGE_NAME="scorum/blockchain:latest"
fi

sudo docker build -t=$IMAGE_NAME .
#sudo docker login --username=$DOCKER_USER --password=$DOCKER_PASS
#sudo docker push $IMAGE_NAME
#sudo docker run -v /var/jenkins_home:/var/jenkins $IMAGE_NAME cp -r /var/cobertura /var/jenkins
#cp -r /var/jenkins_home/cobertura .
