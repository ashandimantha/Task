// Fill out your copyright notice in the Description page of Project Settings.


#include "MyActor.h"

#include <string>

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2DArray.h"
#include "Runtime/RHI/Public/RHI.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture2DDynamic.h"

THIRD_PARTY_INCLUDES_START
#include "Tiffio.h"
#include "Tiff.h"
THIRD_PARTY_INCLUDES_END

// Sets default values
AMyActor::AMyActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMyActor::BeginPlay()
{
	Super::BeginPlay();	
}

void AMyActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

    if (Texture2DArray != nullptr)
    {
        //Texture2DArray->SourceTextures.Empty();
        //Texture2DArray->ReleaseResource();
    }
}

// Called every frame
void AMyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

UTexture2DDynamic* AMyActor::CreateTexture2DDynamicFromChannelData(int32 Width, int32 Height, const TArray<uint8>& ChannelData, FString ChannelDepth)
{
    // Create a new dynamic texture
    UTexture2DDynamic* NewTexture = UTexture2DDynamic::Create(Width, Height, FTexture2DDynamicCreateInfo());
    if (!NewTexture)
    {
        return nullptr;
    }

    // Access the underlying texture resource
    FTexture2DDynamicResource* TextureResource = static_cast<FTexture2DDynamicResource*>(NewTexture->Resource);
    if (!TextureResource)
    {
        return nullptr;
    }

    // Lock the texture for writing
    int32 BytesPerPixel = 4; // For PF_B8G8R8A8 format
    TArray<uint8> Pixels;
    Pixels.SetNum(Width * Height * BytesPerPixel);

    // Copy the channel data to the texture
    for (int32 y = 0; y < Height; y++)
    {
        for (int32 x = 0; x < Width; x++)
        {
            int32 CurrentPixelIndex = (y * Width + x) * BytesPerPixel;
            uint8 CurrentPixelValue = ChannelData[y * Width + x];
            Pixels[CurrentPixelIndex] = CurrentPixelValue;       // Red
            Pixels[CurrentPixelIndex + 1] = CurrentPixelValue;   // Green
            Pixels[CurrentPixelIndex + 2] = CurrentPixelValue;   // Blue
            Pixels[CurrentPixelIndex + 3] = CurrentPixelValue;                 // Alpha
        }
    }

    // Update the texture with new data
    ENQUEUE_RENDER_COMMAND(UpdateDynamicTextureData)(
        [NewTexture, Pixels, Width, Height](FRHICommandListImmediate& RHICmdList)
        {
            FUpdateTextureRegion2D UpdateRegion(0, 0, 0, 0, Width, Height);
            RHIUpdateTexture2D(
                ((FTexture2DDynamicResource*)NewTexture->Resource)->GetTexture2DRHI(),
                0,
                UpdateRegion,
                Width * 4,
                Pixels.GetData()
            );
        }
    );

    FRHITexture2DArray

    Texture2DArray->SourceTextures.Push();
    Texture2DArray->UpdateSourceFromSourceTextures();

    return NewTexture;
}

UTexture2D* AMyActor::ConvertImage(UTexture2DDynamic* DynTex)
{
    if (!DynTex || !DynTex->Resource)
    {
        return nullptr;
    }

    int32 Width = DynTex->SizeX;
    int32 Height = DynTex->SizeY;

    // Create a transient texture
    UTexture2D* ResultTexture = UTexture2D::CreateTransient(Width, Height);
    if (!ResultTexture)
    {
        return nullptr;
    }

    // Ensure the texture is in the correct format and has necessary properties set
    ResultTexture->PlatformData = new FTexturePlatformData();
    ResultTexture->PlatformData->SizeX = Width;
    ResultTexture->PlatformData->SizeY = Height;
    ResultTexture->PlatformData->PixelFormat = PF_R8G8B8A8;

    // Setting up the first mip map level
    FTexture2DMipMap* Mip = new FTexture2DMipMap();
    ResultTexture->PlatformData->Mips.Add(Mip);
    Mip->SizeX = Width;
    Mip->SizeY = Height;

    ResultTexture->Filter = TF_Default; // Set the texture filtering mode
    ResultTexture->SRGB = true; // Set this based on whether your texture is in sRGB space

    ResultTexture->UpdateResource();

    // Copy texture data
    ENQUEUE_RENDER_COMMAND(FConvertTextures)(
        [TextureResource = static_cast<FTexture2DDynamicResource*>(DynTex->Resource), ResultTexture](FRHICommandListImmediate& RHICmdList)
        {
            FTexture2DRHIRef TextureRHI = TextureResource->GetTexture2DRHI();
            uint32 DestStride = 0;
            uint8* ReadData = reinterpret_cast<uint8*>(RHILockTexture2D(TextureRHI, 0, RLM_ReadOnly, DestStride, false));
            if (ReadData)
            {
                FTexture2DMipMap& Mip = ResultTexture->PlatformData->Mips[0];
                void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
                if (Data)
                {
                    FMemory::Memcpy(Data, ReadData, TextureRHI->GetSizeX() * TextureRHI->GetSizeY() * 4);
                    Mip.BulkData.Unlock();
                }
                RHIUnlockTexture2D(TextureRHI, 0, false);
            }
            ResultTexture->UpdateResource();
        });

    return ResultTexture;
}

UTexture2D* AMyActor::CreateTexture2DFromChannelData(int32 Width, int32 Height, const TArray<uint8>& ChannelData, FString ChannelDepth)
{
    FString TextureAssetName = TEXT("AlphaChannel" + ChannelDepth);
    FString PackageName = TEXT("/Game/Assets/AlphaChannel" + ChannelDepth);

    // TODO Need to Save these files With Course Name + ChannelDepth
    // No need to create if we can find the texture asset
	FString AssetPath = FString::Printf(TEXT("/Game/Assets/AlphaChannel%s.AlphaChannel%s"), *ChannelDepth, *ChannelDepth);
	UTexture2D* MyTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *AssetPath));
    if (MyTexture)
    {
	    return MyTexture;
    }

    UPackage* Package = CreatePackage(*PackageName);
	if (!Package) return nullptr;
	Package->FullyLoad();

	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, UTexture2D::StaticClass(), FName(*TextureAssetName), RF_Public | RF_Standalone);
	if (!NewTexture) return nullptr;

    NewTexture->AddToRoot();
	NewTexture->SetPlatformData(new FTexturePlatformData());
	NewTexture->GetPlatformData()->SizeX = Width;
	NewTexture->GetPlatformData()->SizeY = Height;
	NewTexture->GetPlatformData()->PixelFormat = PF_R8G8B8A8;

    uint8* Pixels = new uint8[Width * Height * 4];
	for (int32 y = 0; y < Height; y++)
	{
		for (int32 x = 0; x < Width; x++)
		{
			int32 CurrentPixelIndex = ((y * Width) + x);
            uint8 CurrentPixelValue = ChannelData[CurrentPixelIndex];
			Pixels[4 * CurrentPixelIndex] = CurrentPixelValue;
			Pixels[4 * CurrentPixelIndex + 1] = CurrentPixelValue;
			Pixels[4 * CurrentPixelIndex + 2] = CurrentPixelValue;
			Pixels[4 * CurrentPixelIndex + 3] = CurrentPixelValue;
		}
	}

	// Allocate first mipmap.
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	NewTexture->PlatformData->Mips.Add(Mip);
	Mip->SizeX = Width;
	Mip->SizeY = Height;

	// Lock the texture so it can be modified
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	auto TextureData = (int8*)Mip->BulkData.Realloc(Width * Height * 4);
	FMemory::Memcpy(TextureData, Pixels, sizeof(uint8) * Height * Width * 4);
	Mip->BulkData.Unlock();

    //for (int32 y = 0; y < Height; y++)
    //{
    //    for (int32 x = 0; x < Width; x++)
    //    {
    //        int32 CurrentPixelIndex = y * Width + x;
    //        uint8 CurrentPixelValue = ChannelData[CurrentPixelIndex];
    //        FColor PixelColor(CurrentPixelValue, CurrentPixelValue, CurrentPixelValue, 255);
    //        ((FColor*)TextureData)[CurrentPixelIndex] = PixelColor;
    //    }
    //}

    //NewTexture->PlatformData->Mips[0].BulkData.Unlock();
    NewTexture->Source.Init(Width, Height, 1, 1, ETextureSourceFormat::TSF_BGRA8, Pixels);
    NewTexture->UpdateResource();
    Package->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(NewTexture);
    
    // Save the package
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    bool bSaved = UPackage::SavePackage(Package, NewTexture, EObjectFlags::RF_Public | RF_Standalone, *PackageFileName);
    delete[] Pixels;

    return NewTexture;
}

// Function to extract the 5th channel from a TIFF file
TArray<uint8> AMyActor::ExtractTiffChannels(int& ImageWidth, int& ImageHeight, int DepthIndex)
{
    TArray<uint8> ChannelData;

    FString FullFilePath = FPaths::ProjectDir() / TEXT("Source/Task/cutout.tif");
    std::string FullFilePathStd = TCHAR_TO_UTF8(*FullFilePath);

    TIFF* tif = TIFFOpen(FullFilePathStd.c_str(), "r");
    if (tif)
    {
        uint32 Width, Height, SamplesPerPixel;
        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &Width);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &Height);
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &SamplesPerPixel);
		
        if (SamplesPerPixel < 5)
        {
            // The image does not have enough channels
            TIFFClose(tif);
            return ChannelData;
        }

        // can also have this beauty tdata_t image data reference
        auto Buffer = _TIFFmalloc(TIFFScanlineSize(tif));

        if (!Buffer)
        {
            TIFFClose(tif);
            return ChannelData;
        }

        for (uint32 row = 0; row < Height; row++)
        {
            if (TIFFReadScanline(tif, Buffer, row) == -1)
            {
                // Error reading the scanline 
                break;
            }

            // Cast buffer to the proper type matching the bit depth of the TIFF
            uint8* scanline = static_cast<uint8*>(Buffer);

            for (uint32 col = 0; col < Width; col++)
            {
                // 0-based index, so 4 represents the 5th channel
                int ChannelIndex = (col * SamplesPerPixel) + DepthIndex;
                uint8 ChannelValue = scanline[ChannelIndex];
                ChannelData.Add(ChannelValue);
            }
        }

        _TIFFfree(Buffer);
        TIFFClose(tif);

        ImageWidth = Width;
        ImageHeight = Height;
    }

    return ChannelData;
}

UTexture2DArray* AMyActor::UpdateTexture2DArray(UTexture2D* Texture, int& Size, bool IsRemoving = false)
{
    if (!Texture2DArray)
    {
        UE_LOG(LogTemp, Error, TEXT("Texture2DArray asset is not initialized"));
        return nullptr;
    }

    if (IsRemoving)
    {
        if (Texture2DArray->SourceTextures.Num() == 0)
        {
	        return Texture2DArray;
        }
        int Index = Texture2DArray->SourceTextures.Find(Texture);
        Texture2DArray->SourceTextures.RemoveAt(Index);
        Texture2DArray->UpdateSourceFromSourceTextures();
        Size = Texture2DArray->SourceTextures.Num();
        return Texture2DArray;
    }

    // Add the new texture to the SourceTextures array
    Texture2DArray->SourceTextures.Push(Texture);
    Texture2DArray->UpdateSourceFromSourceTextures();
    Size = Texture2DArray->SourceTextures.Num();

    return Texture2DArray;
}


