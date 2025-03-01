/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2010 Steve Lhomme.  All rights reserved.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** See http://www.gnu.org/licenses/lgpl-2.1.html for LGPL licensing information.**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
  \file
  \author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include <cassert>

#include "matroska/KaxCuesData.h"
#include "matroska/KaxContexts.h"
#include "matroska/KaxBlock.h"
#include "matroska/KaxBlockData.h"
#include "matroska/KaxCluster.h"
#include "matroska/KaxSegment.h"
#include "matroska/KaxDefines.h"
#include "matroska/KaxSemantic.h"

namespace libmatroska {

/*!
  \todo handle codec state checking
  \todo remove duplicate references (reference to 2 frames that each reference the same frame)
*/
void KaxCuePoint::PositionSet(const KaxBlockGroup & BlockReference, std::uint64_t GlobalTimecodeScale)
{
  // fill me
  auto & NewTime = GetChild<KaxCueTime>(*this);
  *static_cast<EbmlUInteger*>(&NewTime) = BlockReference.GlobalTimecode() / GlobalTimecodeScale;

  auto & NewPositions = AddNewChild<KaxCueTrackPositions>(*this);
  auto & TheTrack = GetChild<KaxCueTrack>(NewPositions);
  *static_cast<EbmlUInteger*>(&TheTrack) = BlockReference.TrackNumber();

  auto & TheClustPos = GetChild<KaxCueClusterPosition>(NewPositions);
  *static_cast<EbmlUInteger*>(&TheClustPos) = BlockReference.ClusterPosition();

  // handle reference use
  if (BlockReference.ReferenceCount() != 0) {
    for (unsigned int i=0; i<BlockReference.ReferenceCount(); i++) {
      auto & NewRefs = AddNewChild<KaxCueReference>(NewPositions);
      NewRefs.AddReference(BlockReference.Reference(i).RefBlock(), GlobalTimecodeScale);
    }
  }

  auto CodecState = static_cast<KaxCodecState *>(BlockReference.FindFirstElt(EBML_INFO(KaxCodecState)));
  if (CodecState) {
    auto &CueCodecState = AddNewChild<KaxCueCodecState>(NewPositions);
    *static_cast<EbmlUInteger*>(&CueCodecState) = BlockReference.GetParentCluster()->GetParentSegment()->GetRelativePosition(CodecState->GetElementPosition());
  }

  SetValueIsSet();
}

void KaxCuePoint::PositionSet(const KaxBlockBlob & BlobReference, std::uint64_t GlobalTimecodeScale)
{
  auto &BlockReference = static_cast<KaxInternalBlock&>(BlobReference);
  const KaxBlockGroup *BlockGroupPointer = nullptr;

  if (!BlobReference.IsSimpleBlock()) {
    auto &BlockGroup = static_cast<KaxBlockGroup&>(BlobReference);
    BlockGroupPointer = &BlockGroup;
  }
  PositionSet(BlockReference, BlockGroupPointer, GlobalTimecodeScale);
}

void KaxCuePoint::PositionSet(const KaxSimpleBlock & BlockReference, std::uint64_t GlobalTimecodeScale)
{
  PositionSet(BlockReference, nullptr, GlobalTimecodeScale);
}

void KaxCuePoint::PositionSet(const KaxInternalBlock & BlockReference, const KaxBlockGroup *BlockGroup, std::uint64_t GlobalTimecodeScale)
{
  // fill me
  auto & NewTime = GetChild<KaxCueTime>(*this);
  *static_cast<EbmlUInteger*>(&NewTime) = BlockReference.GlobalTimecode() / GlobalTimecodeScale;

  auto & NewPositions = AddNewChild<KaxCueTrackPositions>(*this);
  auto & TheTrack = GetChild<KaxCueTrack>(NewPositions);
  *static_cast<EbmlUInteger*>(&TheTrack) = BlockReference.TrackNum();

  auto & TheClustPos = GetChild<KaxCueClusterPosition>(NewPositions);
  *static_cast<EbmlUInteger*>(&TheClustPos) = BlockReference.ClusterPosition();

#if 0 // MATROSKA_VERSION >= 2
  // handle reference use
  if (BlockReference.ReferenceCount() != 0) {
    unsigned int i;
    for (i=0; i<BlockReference.ReferenceCount(); i++) {
      KaxCueReference & NewRefs = AddNewChild<KaxCueReference>(NewPositions);
      NewRefs.AddReference(BlockReference.Reference(i).RefBlock(), GlobalTimecodeScale);
    }
  }
#endif // MATROSKA_VERSION

  if (BlockGroup) {
    const auto CodecState = static_cast<const KaxCodecState *>(BlockGroup->FindFirstElt(EBML_INFO(KaxCodecState)));
    if (CodecState) {
      auto &CueCodecState = AddNewChild<KaxCueCodecState>(NewPositions);
      *static_cast<EbmlUInteger*>(&CueCodecState) = BlockGroup->GetParentCluster()->GetParentSegment()->GetRelativePosition(CodecState->GetElementPosition());
    }
  }

  SetValueIsSet();
}

/*!
  \todo handle codec state checking
*/
void KaxCueReference::AddReference(const KaxBlockBlob & BlockReference, std::uint64_t GlobalTimecodeScale)
{
  auto& theBlock = static_cast<KaxInternalBlock&>(BlockReference);
  auto& NewTime = GetChild<KaxCueRefTime>(*this);
  *static_cast<EbmlUInteger*>(&NewTime) = theBlock.GlobalTimecode() / GlobalTimecodeScale;

  auto & TheClustPos = GetChild<KaxCueRefCluster>(*this);
  *static_cast<EbmlUInteger*>(&TheClustPos) = theBlock.ClusterPosition();
}

bool KaxCuePoint::IsSmallerThan(const EbmlElement * Cmp) const
{
  assert(EbmlId(*this) == EBML_ID(KaxCuePoint));
  assert(EbmlId(*Cmp)  == EBML_ID(KaxCuePoint));

  auto theCmp = static_cast<const KaxCuePoint *>(Cmp);

  // compare timecode
  auto TimeCodeA = static_cast<const KaxCueTime *>(FindElt(EBML_INFO(KaxCueTime)));
  if (!TimeCodeA)
    return false;

  auto TimeCodeB = static_cast<const KaxCueTime *>(theCmp->FindElt(EBML_INFO(KaxCueTime)));
  if (!TimeCodeB)
    return false;

  if (TimeCodeA->IsSmallerThan(TimeCodeB))
    return true;

  if (TimeCodeB->IsSmallerThan(TimeCodeA))
    return false;

  // compare tracks (timecodes are equal)
  const auto TrackA = static_cast<const KaxCueTrack *>(FindElt(EBML_INFO(KaxCueTrack)));
  if (!TrackA)
    return false;

  const auto TrackB = static_cast<const KaxCueTrack *>(theCmp->FindElt(EBML_INFO(KaxCueTrack)));
  if (!TrackB)
    return false;

  if (TrackA->IsSmallerThan(TrackB))
    return true;

  if (TrackB->IsSmallerThan(TrackA))
    return false;

  return false;
}

bool KaxCuePoint::Timecode(std::uint64_t & aTimecode, std::uint64_t GlobalTimecodeScale) const
{
  const auto aTime = static_cast<const KaxCueTime *>(FindFirstElt(EBML_INFO(KaxCueTime)));
  if (!aTime)
    return false;
  aTimecode = static_cast<std::uint64_t>(*aTime) * GlobalTimecodeScale;
  return true;
}

/*!
  \brief return the position of the Cluster to load
*/
const KaxCueTrackPositions * KaxCuePoint::GetSeekPosition() const
{
  const KaxCueTrackPositions * result = nullptr;
  std::uint64_t aPosition = 0xFFFFFFFFFFFFFFFLL;
  // find the position of the "earlier" Cluster
  auto aPoss = static_cast<const KaxCueTrackPositions *>(FindFirstElt(EBML_INFO(KaxCueTrackPositions)));
  while (aPoss) {
    auto aPos = static_cast<const KaxCueClusterPosition *>(aPoss->FindFirstElt(EBML_INFO(KaxCueClusterPosition)));
    if (aPos && static_cast<std::uint64_t>(*aPos) < aPosition) {
      aPosition = static_cast<std::uint64_t>(*aPos);
      result = aPoss;
    }

    aPoss = static_cast<const KaxCueTrackPositions *>(FindNextElt(*aPoss));
  }
  return result;
}

std::uint64_t KaxCueTrackPositions::ClusterPosition() const
{
  const auto aPos = static_cast<const KaxCueClusterPosition *>(FindFirstElt(EBML_INFO(KaxCueClusterPosition)));
  if (!aPos)
    return 0;

  return static_cast<std::uint64_t>(*aPos);
}

std::uint16_t KaxCueTrackPositions::TrackNumber() const
{
  const auto aTrack = static_cast<const KaxCueTrack *>(FindFirstElt(EBML_INFO(KaxCueTrack)));
  if (!aTrack)
    return 0;

  return static_cast<std::uint16_t>(*aTrack);
}


} // namespace libmatroska
