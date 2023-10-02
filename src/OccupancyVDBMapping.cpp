// this is for emacs file handling -*- mode: c++; indent-tabs-mode: nil -*-

// -- BEGIN LICENSE BLOCK ----------------------------------------------
// Copyright 2021 FZI Forschungszentrum Informatik
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// -- END LICENSE BLOCK ------------------------------------------------

//----------------------------------------------------------------------
/*!\file
 *
 * \author  Marvin Große Besselmann grosse@fzi.de
 * \author  Lennart Puck puck@fzi.de
 * \date    2021-04-29
 *
 */
//----------------------------------------------------------------------


#include "vdb_mapping/OccupancyVDBMapping.h"

namespace vdb_mapping {

bool OccupancyVDBMapping::updateFreeNode(Voxel& voxel_value, bool& active)
{
  voxel_value.probs += m_logodds_miss;
  if (voxel_value.probs < m_logodds_thres_min)
  {
    active = false;
    if (voxel_value.probs < m_min_logodds)
    {
      voxel_value.probs = m_min_logodds;
    }
  }
  return true;
}

void OccupancyVDBMapping::updateSemanticLabel(Voxel& voxel_value, const Voxel& update_value) 
{
    // Update the queue for semantic labels
    voxel_value.s_h.push(update_value.semantic);
    if (voxel_value.s_h.size() > m_queue_size)
    {
        voxel_value.s_h.pop();
    }

    // Find the most frequent semantic label and update the voxel_value.semantic
    std::unordered_map<int, int> s_map;
    std::queue<int> temp_queue = voxel_value.s_h;  // Create a copy of the original queue

    while (!temp_queue.empty())
    {
        s_map[temp_queue.front()]++;
        temp_queue.pop();
    }

    int max = 0;
    for (auto& it : s_map)
    {
        if (it.second > max)
        {
            max = it.second;
            voxel_value.semantic = it.first;
        }
    }
}

bool OccupancyVDBMapping::updateOccupiedNode(Voxel& update_value, Voxel& voxel_value, bool& active)
{
  voxel_value.probs += m_logodds_hit;
  if (voxel_value.probs > m_logodds_thres_max)
  {
    active = true;
    if (voxel_value.probs > m_max_logodds)
    {
      voxel_value.probs = m_max_logodds;
    }
  }
  if (active)
  {
    // normalize color
    const float s = update_value.r + update_value.g + update_value.b;
    update_value.r /= s;
    update_value.g /= s;
    update_value.b /= s;
    // update color
    voxel_value.r = (voxel_value.r + update_value.r) / 2;
    voxel_value.g = (voxel_value.g + update_value.g) / 2;
    voxel_value.b = (voxel_value.b + update_value.b) / 2;
    // update the queue for semantic labels
    updateSemanticLabel(voxel_value, update_value);
  }

  return true;
}


void OccupancyVDBMapping::setConfig(const Config& config)
{
  // call base class function
  VDBMapping::setConfig(config);

  // Sanity Check for input config
  if (config.prob_miss > 0.5)
  {
    std::cerr << "Probability for a miss should be below 0.5 but is " << config.prob_miss
              << std::endl;
    return;
  }
  if (config.prob_hit < 0.5)
  {
    std::cerr << "Probability for a hit should be above 0.5 but is " << config.prob_hit
              << std::endl;
    return;
  }

  // Store probabilities as log odds
  m_logodds_miss = static_cast<float>(log(config.prob_miss) - log(1 - config.prob_miss));
  m_logodds_hit  = static_cast<float>(log(config.prob_hit) - log(1 - config.prob_hit));
  m_logodds_thres_min =
    static_cast<float>(log(config.prob_thres_min) - log(1 - config.prob_thres_min));
  m_logodds_thres_max =
    static_cast<float>(log(config.prob_thres_max) - log(1 - config.prob_thres_max));
  // Values to clamp the logodds in order to prevent non dynamic map behavior
  m_max_logodds = static_cast<float>(log(0.99) - log(0.01));
  m_min_logodds = static_cast<float>(log(0.01) - log(0.99));
  m_config_set  = true;
}

} // namespace vdb_mapping
