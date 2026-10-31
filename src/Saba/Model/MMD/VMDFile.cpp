//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "VMDFile.h"

#include <Saba/Base/Log.h>
#include <Saba/Base/File.h>

namespace saba
{
	namespace
	{
		template <typename T>
		bool Read(T* val, File& file)
		{
			return file.Read(val);
		}

		// 1 件あたりの直列化サイズ（バイト数）。VMDFile.h の各構造体のフィールド合計。
		constexpr size_t kMotionRecordSize = 15 + 4 + 12 + 16 + 64;			// boneName, frame, translate, quaternion, interpolation
		constexpr size_t kMorphRecordSize = 15 + 4 + 4;						// blendShapeName, frame, weight
		constexpr size_t kCameraRecordSize = 4 + 4 + 12 + 12 + 24 + 4 + 1;	// frame, distance, interest, rotate, interpolation, viewAngle, isPerspective
		constexpr size_t kLightRecordSize = 4 + 12 + 12;					// frame, color, position
		constexpr size_t kShadowRecordSize = 4 + 1 + 4;						// frame, shadowType, distance
		constexpr size_t kIkRecordMinSize = 4 + 1 + 4;						// frame, show, ikInfoCount（IK 情報は可変長なので最小値）
		constexpr size_t kIkInfoRecordSize = 20 + 1;						// name, enable

		// 件数をそのまま信用すると、壊れたファイルが巨大な resize を引き起こして落ちる。
		// ファイルの残りバイト数からあり得る上限を計算し、超えていたら読み込みエラーにする。
		bool CheckCount(uint32_t count, size_t recordSize, File& file, const char* name)
		{
			const File::Offset remaining = file.GetSize() - file.Tell();
			const uint64_t maxCount = remaining > 0 ? static_cast<uint64_t>(remaining) / static_cast<uint64_t>(recordSize) : 0;
			if (static_cast<uint64_t>(count) > maxCount)
			{
				SABA_WARN("VMD {} count {} exceeds the limit {} computed from the remaining file size.", name, count, maxCount);
				return false;
			}
			return true;
		}

		bool ReadHeader(VMDFile* vmd, File& file)
		{
			Read(&vmd->m_header.m_header, file);
			Read(&vmd->m_header.m_modelName, file);

			if (vmd->m_header.m_header.ToString() != "Vocaloid Motion Data 0002" &&
				vmd->m_header.m_header.ToString() != "Vocaloid Motion Data"
				)
			{
				SABA_WARN("VMD Header error.");
				return false;
			}

			return !file.IsBad();
		}

		bool ReadMotion(VMDFile* vmd, File& file)
		{
			uint32_t motionCount = 0;
			if (!Read(&motionCount, file))
			{
				return false;
			}

			if (!CheckCount(motionCount, kMotionRecordSize, file, "Motion"))
			{
				return false;
			}

			vmd->m_motions.resize(motionCount);
			for (auto& motion : vmd->m_motions)
			{
				Read(&motion.m_boneName, file);
				Read(&motion.m_frame, file);
				Read(&motion.m_translate, file);
				Read(&motion.m_quaternion, file);
				Read(&motion.m_interpolation, file);
			}

			return !file.IsBad();
		}

		bool ReadBlendShape(VMDFile* vmd, File& file)
		{
			uint32_t blendShapeCount = 0;
			if (!Read(&blendShapeCount, file))
			{
				return false;
			}

			if (!CheckCount(blendShapeCount, kMorphRecordSize, file, "BlendShape"))
			{
				return false;
			}

			vmd->m_morphs.resize(blendShapeCount);
			for (auto& morph : vmd->m_morphs)
			{
				Read(&morph.m_blendShapeName, file);
				Read(&morph.m_frame, file);
				Read(&morph.m_weight, file);
			}

			return !file.IsBad();
		}

		bool ReadCamera(VMDFile* vmd, File& file)
		{
			uint32_t cameraCount = 0;
			if (!Read(&cameraCount, file))
			{
				return false;
			}

			if (!CheckCount(cameraCount, kCameraRecordSize, file, "Camera"))
			{
				return false;
			}

			vmd->m_cameras.resize(cameraCount);
			for (auto& camera : vmd->m_cameras)
			{
				Read(&camera.m_frame, file);
				Read(&camera.m_distance, file);
				Read(&camera.m_interest, file);
				Read(&camera.m_rotate, file);
				Read(&camera.m_interpolation, file);
				Read(&camera.m_viewAngle, file);
				Read(&camera.m_isPerspective, file);
			}

			return !file.IsBad();
		}

		bool ReadLight(VMDFile* vmd, File& file)
		{
			uint32_t lightCount = 0;
			if (!Read(&lightCount, file))
			{
				return false;
			}

			if (!CheckCount(lightCount, kLightRecordSize, file, "Light"))
			{
				return false;
			}

			vmd->m_lights.resize(lightCount);
			for (auto& light : vmd->m_lights)
			{
				Read(&light.m_frame, file);
				Read(&light.m_color, file);
				Read(&light.m_position, file);
			}

			return !file.IsBad();
		}

		bool ReadShadow(VMDFile* vmd, File& file)
		{
			uint32_t shadowCount = 0;
			if (!Read(&shadowCount, file))
			{
				return false;
			}

			if (!CheckCount(shadowCount, kShadowRecordSize, file, "Shadow"))
			{
				return false;
			}

			vmd->m_shadows.resize(shadowCount);
			for (auto& shadow : vmd->m_shadows)
			{
				Read(&shadow.m_frame, file);
				Read(&shadow.m_shadowType, file);
				Read(&shadow.m_distance, file);
			}

			return !file.IsBad();
		}

		bool ReadIK(VMDFile* vmd, File& file)
		{
			uint32_t ikCount = 0;
			if (!Read(&ikCount, file))
			{
				return false;
			}

			if (!CheckCount(ikCount, kIkRecordMinSize, file, "IK"))
			{
				return false;
			}

			vmd->m_iks.resize(ikCount);
			for (auto& ik : vmd->m_iks)
			{
				Read(&ik.m_frame, file);
				Read(&ik.m_show, file);
				uint32_t ikInfoCount = 0;
				if (!Read(&ikInfoCount, file))
				{
					return false;
				}
				if (!CheckCount(ikInfoCount, kIkInfoRecordSize, file, "IKInfo"))
				{
					return false;
				}
				ik.m_ikInfos.resize(ikInfoCount);
				for (auto& ikInfo : ik.m_ikInfos)
				{
					Read(&ikInfo.m_name, file);
					Read(&ikInfo.m_enable, file);
				}
			}

			return !file.IsBad();
		}

		bool ReadVMDFile(VMDFile* vmd, File& file)
		{
			if (!ReadHeader(vmd, file))
			{
				SABA_WARN("ReadHeader Fail.");
				return false;
			}

			if (!ReadMotion(vmd, file))
			{
				SABA_WARN("ReadMotion Fail.");
				return false;
			}

			if (file.Tell() < file.GetSize())
			{
				if (!ReadBlendShape(vmd, file))
				{
					SABA_WARN("ReadBlednShape Fail.");
					return false;
				}
			}

			if (file.Tell() < file.GetSize())
			{
				if (!ReadCamera(vmd, file))
				{
					SABA_WARN("ReadCamera Fail.");
					return false;
				}
			}

			if (file.Tell() < file.GetSize())
			{
				if (!ReadLight(vmd, file))
				{
					SABA_WARN("ReadLight Fail.");
					return false;
				}
			}

			if (file.Tell() < file.GetSize())
			{
				if (!ReadShadow(vmd, file))
				{
					SABA_WARN("ReadShadow Fail.");
					return false;
				}
			}

			if (file.Tell() < file.GetSize())
			{
				if (!ReadIK(vmd, file))
				{
					SABA_WARN("ReadIK Fail.");
					return false;
				}
			}

			return true;
		}
	}

	bool ReadVMDFile(VMDFile * vmd, const char * filename)
	{
		File file;
		if (!file.Open(filename))
		{
			SABA_WARN("VMD File Open Fail. {}", filename);
			return false;
		}

		return ReadVMDFile(vmd, file);
	}

}
