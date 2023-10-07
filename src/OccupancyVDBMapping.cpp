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
  voxel_value[0][0] += m_logodds_miss;
  if (voxel_value[0][0] < m_logodds_thres_min)
  {
    active = false;
    if (voxel_value[0][0] < m_min_logodds)
    {
      voxel_value[0][0] = m_min_logodds;
    }
  }
  return true;
}

bool OccupancyVDBMapping::updateOccupiedNode(Voxel& voxel_value, bool& active, const Voxel& new_value)
{
  voxel_value[0][0] += m_logodds_hit;
  if (voxel_value[0][0] > m_logodds_thres_max)
  {
    active = true;
    if (voxel_value[0][0] > m_max_logodds)
    {
      voxel_value[0][0] = m_max_logodds;
    }
    updateVoxelField(voxel_value, new_value);
  }
  return true;
}

Voxel OccupancyVDBMapping::craeteVoxelFromPoint(const PointT& point)
{
  Voxel voxel;
  voxel.setZero();
  const float s = (point.r + point.g + point.b);
  if (s < 1e-5)
  {
    return voxel; // no color information
  }
  voxel[0][1] = point.r / s * 3.0;
  voxel[0][2] = point.g / s * 3.0;
  voxel[0][3] = point.b / s * 3.0;
  return voxel;
}

void OccupancyVDBMapping::updateVoxelField(Voxel& cur_voxel, const Voxel& new_voxel)
{
  // average the color information
  cur_voxel[0][1] = (cur_voxel[0][1] + new_voxel[0][1]) / 2.0;
  cur_voxel[0][2] = (cur_voxel[0][2] + new_voxel[0][2]) / 2.0;
  cur_voxel[0][3] = (cur_voxel[0][3] + new_voxel[0][3]) / 2.0;
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
