/*
 * Copyright (c) 2008, Maxim Likhachev
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of Pennsylvania nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>

#include <ros/ros.h>
#include <ros/package.h>

using namespace std;

#include <sbpl/headers.h>

int planxythetalat(const char* envCfgFilename, const char* motPrimFilename, const char* eps)
{
    int bRet = 0;
    double allocated_time_secs = 100.0; // in seconds
    double initialEpsilon = 10;//atof(eps);
    MDPConfig MDPCfg;
    bool bsearchuntilfirstsolution = false;
    bool bforwardsearch = true;

    // set the perimeter of the robot (it is given with 0,0,0 robot ref. point for which planning is done)
    vector<sbpl_2Dpt_t> perimeterptsV;
    sbpl_2Dpt_t pt_m;
    double halfwidth = 0.1;
    double halflength = 0.1;
    pt_m.x = -halflength;
    pt_m.y = -halfwidth;
    perimeterptsV.push_back(pt_m);
    pt_m.x = halflength;
    pt_m.y = -halfwidth;
    perimeterptsV.push_back(pt_m);
    pt_m.x = halflength;
    pt_m.y = halfwidth;
    perimeterptsV.push_back(pt_m);
    pt_m.x = -halflength;
    pt_m.y = halfwidth;
    perimeterptsV.push_back(pt_m);

    // perimeterptsV.clear();

    // Initialize Environment (should be called before initializing anything else)
    EnvironmentNAVXYTHETALAT environment_navxythetalat;

    if (!environment_navxythetalat.InitializeEnv(envCfgFilename, perimeterptsV, motPrimFilename)) {
        printf("ERROR: InitializeEnv failed\n");
        throw new SBPL_Exception();
    }

    // Initialize MDP Info
    if (!environment_navxythetalat.InitializeMDPCfg(&MDPCfg)) {
        printf("ERROR: InitializeMDPCfg failed\n");
        throw new SBPL_Exception();
    }

    // plan a path
    vector<int> solution_stateIDs_V;

    SBPLPlanner* planner = new ARAPlanner(&environment_navxythetalat, bforwardsearch);    
    
    // set planner properties
    if (planner->set_start(MDPCfg.startstateid) == 0) {
        printf("ERROR: failed to set start state\n");
        throw new SBPL_Exception();
    }
    if (planner->set_goal(MDPCfg.goalstateid) == 0) {
        printf("ERROR: failed to set goal state\n");
        throw new SBPL_Exception();
    }

    planner->set_initialsolution_eps(initialEpsilon);
    planner->set_search_mode(bsearchuntilfirstsolution);

    environment_navxythetalat.InitViz();
    environment_navxythetalat.VisualizeMap();

    // plan
    printf("start planning...\n");
    bRet = planner->replan(allocated_time_secs, &solution_stateIDs_V);
    printf("done planning\n");
    printf("size of solution=%d\n", (unsigned int)solution_stateIDs_V.size());

    environment_navxythetalat.VisualizePath(solution_stateIDs_V);

    environment_navxythetalat.PrintTimeStat(stdout);

    // print a path
    if (bRet) {
        // print the solution
        printf("Solution is found\n");
    }
    else {
        printf("Solution does not exist\n");
    }

    fflush(NULL);

    delete planner;

    return bRet;
}

int planxythetacont(const char* envCfgFilename, int ompl_id)
{
    int planner_id = ompl_id;
    double allocated_time_secs = 100.0;

    OMPLPlanner ompl_planner(planner_id, allocated_time_secs);

    if (!ompl_planner.initEnv(envCfgFilename)) {
        printf("ERROR: InitializeEnv failed\n");
        throw new SBPL_Exception();
    }

    if (!ompl_planner.initOMPL()) {
        printf("ERROR: Initializing OMPL failed\n");
        throw new SBPL_Exception();
    }

    bool bRet = ompl_planner.plan();

    return bRet;
}


int main(int argc, char *argv[])
{
    ros::init(argc, argv, "kinematic_test");

    const char* configFilename;
    const char* motPrimFilename;
    const char* eps;

    ros::NodeHandle ph("~");
    std::string r_config_path, r_prim_path;
    ph.getParam("config_path", r_config_path);
    ph.getParam("prim_path", r_prim_path);

    std::string lib_path = ros::package::getPath("sbpl_lh");
    std::string config_path = lib_path + r_config_path; 
    std::string prim_path = lib_path + r_prim_path; 

    if(argc <= 1) {
        configFilename = config_path.c_str();
        motPrimFilename = prim_path.c_str();
    }
    else {
        configFilename = argv[1];
        motPrimFilename = argv[2];
        eps = argv[3];
    }

    bool use_ompl;
    int ompl_id;

    ph.getParam("use_ompl", use_ompl);  
    ph.getParam("ompl_id", ompl_id);

    int plannerRes;

    if(!use_ompl)
        plannerRes = planxythetalat(configFilename, motPrimFilename, eps);
    else
        plannerRes = planxythetacont(configFilename, ompl_id);        

    return plannerRes;
}