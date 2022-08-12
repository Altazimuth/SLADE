
class SIFRottGfxBase : public SIFormat
{
public:
	SIFRottGfxBase(string_view id, string_view name, int reliability) :
		SIFormat(id, name, "dat", reliability)
	{
	}
	~SIFRottGfxBase() = default;

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Read header
		auto header   = (const gfx::ROTTPatchHeader*)mc.data();
		info.width    = wxINT16_SWAP_ON_BE(header->width);
		info.height   = wxINT16_SWAP_ON_BE(header->height);
		info.offset_x = wxINT16_SWAP_ON_BE(header->left) + (wxINT16_SWAP_ON_BE(header->origsize) / 2);
		info.offset_y = wxINT16_SWAP_ON_BE(header->top) + wxINT16_SWAP_ON_BE(header->origsize);

		// Setup other info
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readRottGfx(SImage& image, MemChunk& data, bool mask)
	{
		// Get image info
		auto info = this->info(data, 0);

		// Setup variables
		size_t hdr_size   = sizeof(gfx::ROTTPatchHeader);
		short  translevel = 255;
		if (mask)
		{
			translevel = data.readL16(hdr_size);
			hdr_size += 2;
		}

		// Read column offsets
		vector<uint16_t> col_offsets(info.width);
		auto             c_ofs = (const uint16_t*)(data.data() + hdr_size);
		for (int a = 0; a < info.width; a++)
			col_offsets[a] = wxUINT16_SWAP_ON_BE(c_ofs[a]);

		// Create image
		image.create(info);

		// Load data
		auto img_data = imageData(image);
		auto img_mask = imageMask(image);
		for (int c = 0; c < info.width; c++)
		{
			// Get current column offset
			uint16_t col_offset = col_offsets[c];

			// Check column offset is valid
			if (col_offset >= (unsigned)data.size())
				return false;

			// Go to start of column
			auto bits = data.data() + col_offset;

			// Read posts
			int counter = 0;
			while (true)
			{
				// Get row offset
				uint8_t row = *bits++;

				if (row == 0xFF) // End of column?
					break;

				// Get no. of pixels
				uint8_t n_pix = *bits++;
				for (uint8_t p = 0; p < n_pix; p++)
				{
					// Get pixel position
					int pos = ((row + p) * info.width + c);

					// Stop if we're outside the image
					if (pos > info.width * info.height)
						break;

					// Stop if for some reason we're outside the gfx data
					if (bits > data.data() + data.size())
						break;

					// Fail if bogus data gives a negative pos (this corrupts the heap!)
					if (pos < 0)
						return false;

					// Write pixel data
					if (mask && *bits == 254)
					{
						img_data[pos] = 0;
						img_mask[pos] = translevel;
					}
					else
					{
						img_data[pos] = *bits++;
						img_mask[pos] = 0xFF;
					}
				}
				if (mask && *bits == 254)
					++bits;
			}
		}

		return true;
	}

	// TODO: Actually put in support for ROTT masked patches into this function
	bool writeRottGfx(SImage& image, MemChunk& out, Palette* pal, bool masked)
	{
		// Convert image to column/post structure
		vector<Column> columns;
		auto           data = imageData(image);
		auto           mask = imageMask(image);

		// Go through columns
		uint32_t offset = 0;
		for (int c = 0; c < image.width(); c++)
		{
			Column col;
			Post   post;
			post.row_off = 0;
			bool ispost  = false;

			offset          = c;
			uint8_t row_off = 0;
			for (int r = 0; r < image.height(); r++)
			{
				// For vanilla-compatible dimensions, use a split at 128 to prevent tiling.
				if (image.height() < 256)
				{
					if (row_off == 128)
					{
						// Finish current post if any
						if (ispost)
						{
							col.posts.push_back(post);
							post.pixels.clear();
							ispost = false;
						}
					}
				}

				// If the current pixel is not transparent, add it to the current post
				if (!mask || mask[offset] > 0)
				{
					// If we're not currently building a post, begin one and set its offset
					if (!ispost)
					{
						// Set offset
						post.row_off = row_off;

						// Start post
						ispost = true;
					}

					// Add the pixel to the post
					post.pixels.push_back(data[offset]);
				}
				else if (ispost)
				{
					// If the current pixel is transparent and we are currently building
					// a post, add the current post to the list and clear it
					col.posts.push_back(post);
					post.pixels.clear();
					ispost = false;
				}

				// Go to next row
				offset += image.width();
				row_off++;
			}

			// If the column ended with a post, add it
			if (ispost)
				col.posts.push_back(post);

			// Add the column data
			columns.push_back(col);

			// Go to next column
			offset++;
		}

		// Write ROTT gfx data to output
		out.clear();
		out.seek(0, SEEK_SET);

		// Setup header
		gfx::ROTTPatchHeader header;
		header.origsize = image.width();
		header.top      = image.offset().y - image.width();
		header.left     = image.offset().x - (image.width() / 2);
		header.width    = image.width();
		header.height   = image.height();

		// Byteswap header values if needed
		header.origsize = wxINT16_SWAP_ON_BE(header.origsize);
		header.top      = wxINT16_SWAP_ON_BE(header.top);
		header.left     = wxINT16_SWAP_ON_BE(header.left);
		header.width    = wxINT16_SWAP_ON_BE(header.width);
		header.height   = wxINT16_SWAP_ON_BE(header.height);

		// Write it
		out.write(&header.origsize, 2);
		out.write(&header.width, 2);
		out.write(&header.height, 2);
		out.write(&header.left, 2);
		out.write(&header.top, 2);

		// Write dummy column offsets for now
		vector<uint16_t> col_offsets(columns.size());
		out.write(col_offsets.data(), columns.size() * 2);

		// Write columns
		for (size_t c = 0; c < columns.size(); c++)
		{
			// Record column offset
			col_offsets[c] = wxUINT16_SWAP_ON_BE(out.currentPos());

			// Determine column size (in bytes)
			uint32_t col_size = 0;
			for (auto& post : columns[c].posts)
				col_size += post.pixels.size() + 2;

			// Allocate memory to write the column data
			out.reSize(out.size() + col_size, true);

			// Write column posts
			for (auto& post : columns[c].posts)
			{
				// Write row offset
				out.write(&post.row_off, 1);

				// Write no. of pixels
				uint8_t npix = post.pixels.size();
				out.write(&npix, 1);

				// Write pixels
				for (auto& pixel : post.pixels)
					out.write(&pixel, 1);
			}

			// Write '255' row to signal end of column
			uint8_t temp = 255;
			out.write(&temp, 1);
		}

		// Now we write column offsets
		out.seek(sizeof(gfx::ROTTPatchHeader), SEEK_SET);
		out.write(col_offsets.data(), columns.size() * 2);

		// Allocate memory to write dummy data at the end,
		// solely to stop ROTT patches being read as Doom beta ones
		constexpr int NUM_DUMMY_BYTES = 16;
		const uint8_t padding_value = 0xAB;
		out.reSize(out.size() + NUM_DUMMY_BYTES, true);
		out.seek(NUM_DUMMY_BYTES, SEEK_END);
		for (int i = 0; i < NUM_DUMMY_BYTES; i++)
			out.write(&padding_value, 1);

		return true;
	}

private:
	// ROTT Gfx format structs
	struct Post
	{
		uint8_t         row_off;
		vector<uint8_t> pixels;
	};

	struct Column
	{
		vector<Post> posts;
	};
};

class SIFRottGfx : public SIFRottGfxBase
{
public:
	SIFRottGfx() : SIFRottGfxBase("rott", "ROTT Gfx", 121) {}
	~SIFRottGfx() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		return EntryDataFormat::format("img_rott")->isThisFormat(mc) >= EntryDataFormat::MATCH_PROBABLY;
	}

	Writable canWrite(SImage& image) override
	{
		// Must be converted to paletted to be written
		if (image.type() == SImage::Type::PalMask)
			return Writable::Yes;
		else
			return Writable::Convert;
	}

	bool canWriteType(SImage::Type type) override
	{
		// ROTT format gfx can only be written as paletted
		if (type == SImage::Type::PalMask)
			return true;
		else
			return false;
	}

	bool convertWritable(SImage& image, ConvertOptions opt) override
	{
		// Do mask conversion
		if (!opt.transparency)
			image.fillAlpha(255);
		else if (opt.mask_source == Mask::Colour)
			image.maskFromColour(opt.mask_colour, opt.pal_target);
		else if (opt.mask_source == Mask::Alpha)
			image.cutoffMask(opt.alpha_threshold);

		// Convert to paletted
		image.convertPaletted(opt.pal_target, opt.pal_current);

		return true;
	}

private:
	bool readImage(SImage& image, MemChunk& data, int index) override { return readRottGfx(image, data, false); }

	bool writeImage(SImage &image, MemChunk &out, Palette *pal, int index) override
	{
		return writeRottGfx(image, out, pal, false);
	}
};

class SIFRottGfxMasked : public SIFRottGfxBase
{
public:
	SIFRottGfxMasked() : SIFRottGfxBase("rottmask", "ROTT Masked Gfx", 120) {}
	~SIFRottGfxMasked() = default;

	bool isThisFormat(MemChunk& mc) override { return EntryDataFormat::format("img_rottmask")->isThisFormat(mc); }

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override { return readRottGfx(image, data, true); }
};

class SIFRottLbm : public SIFormat
{
public:
	SIFRottLbm() : SIFormat("rottlbm", "ROTT Lbm", "dat", 80) {}
	~SIFRottLbm() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		return EntryDataFormat::format("img_rottlbm")->isThisFormat(mc) >= EntryDataFormat::MATCH_PROBABLY;
	}

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Setup info
		info.width       = mc.readL16(0);
		info.height      = mc.readL16(2);
		info.colformat   = SImage::Type::PalMask;
		info.has_palette = true;
		info.format      = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Get image info
		auto info = this->info(data, index);

		// ROTT source code says: "LIMITATIONS - Only works with 320x200!!!"
		if (info.width != 320 || info.height != 200)
			return false;

		// Build palette
		Palette palette;
		for (size_t c = 0; c < 256; ++c)
		{
			ColRGBA color;
			color.r = data[(c * 3) + 4];
			color.g = data[(c * 3) + 5];
			color.b = data[(c * 3) + 6];
			palette.setColour(c, color);
		}

		// Create image
		image.create(info, &palette);
		auto img_data = imageData(image);
		image.fillAlpha(255);

		// Create some variables needed for LBM decompression
		auto    read    = data.data() + 768 + 4;
		auto    readend = data.data() + data.size();
		auto    dest    = img_data;
		auto    destend = img_data + (info.width * info.height);
		uint8_t code    = 0;
		uint8_t length  = 0;
		uint8_t count   = 0;

		// Read image data
		while (read < readend && dest < destend && count < info.width)
		{
			code = *read++;
			if (code < 0x80)
			{
				length = code + 1;
				memcpy(dest, read, length);
				dest += length;
				read += length;
			}
			else if (code > 0x80)
			{
				length = (code ^ 0xFF) + 2;
				;
				code = *read++;
				memset(dest, code, length);
				dest += length;
			}
			else
				length = 0;

			count += length;
		}

		return true;
	}
};

class SIFRottRaw : public SIFormat
{
public:
	SIFRottRaw() : SIFormat("rottraw", "ROTT Raw", "dat", 101) {}
	~SIFRottRaw() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		return EntryDataFormat::format("img_rottraw")->isThisFormat(mc) >= EntryDataFormat::MATCH_PROBABLY;
	}

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Read header
		auto header   = (const gfx::PatchHeader*)mc.data();
		info.width    = wxINT16_SWAP_ON_BE(header->width);
		info.height   = wxINT16_SWAP_ON_BE(header->height);
		info.offset_x = wxINT16_SWAP_ON_BE(header->left);
		info.offset_y = wxINT16_SWAP_ON_BE(header->top);

		// Set other info
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Get image info
		auto info = this->info(data, index);

		// Create image (swapped width/height because column-major)
		image.create(info.height, info.width, SImage::Type::PalMask);
		image.fillAlpha(255);

		// Read raw pixel data
		data.read(imageData(image), info.width * info.height, 8);

		// Convert from column-major to row-major
		image.rotate(90);
		image.mirror(true);

		return true;
	}
};

class SIFRottPic : public SIFormat
{
public:
	SIFRottPic() : SIFormat("rottpic", "ROTT Picture", "dat", 60) {}
	~SIFRottPic() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		return EntryDataFormat::format("img_rottpic")->isThisFormat(mc) >= EntryDataFormat::MATCH_PROBABLY;
	}

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Read dimensions
		info.width  = mc[0] * 4;
		info.height = mc[1];

		// Setup other info
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Get image info
		auto info = this->info(data, index);

		// Check data
		if ((int)data.size() != 4 + info.width * info.height)
			return false;

		// Create image
		image.create(info);
		auto img_data = imageData(image);
		auto img_mask = imageMask(image);

		// Read data
		auto entryend = data.data() + data.size() - 2;
		auto dataend  = img_data + data.size() - 4;
		auto pixel    = data.data() + 2;
		auto brush    = img_data;
		while (pixel < entryend)
		{
			*brush = *pixel++;
			brush += 4;
			if (brush >= dataend)
				brush -= data.size() - 5;
		}

		// Create mask (index 255 is transparent)
		for (int a = 0; a < info.width * info.height; a++)
		{
			if (img_data[a] == 255)
				img_mask[a] = 0;
			else
				img_mask[a] = 255;
		}

		return true;
	}
};

class SIFRottWall : public SIFormat
{
public:
	SIFRottWall() : SIFormat("rottwall", "ROTT Flat", "dat", 10) {}
	~SIFRottWall() = default;

	bool isThisFormat(MemChunk& mc) override { return (mc.size() == 4096 || mc.size() == 51200); }

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Always the same thing
		info.width     = mc.size() == 4096 ? 64 : 256;
		info.height    = mc.size() == 4096 ? 64 : 200;
		info.offset_x  = 0;
		info.offset_y  = 0;
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Get image info
		auto info = this->info(data, index);

		// Create image (swapped width/height because column-major)
		image.create(info.height, info.width, SImage::Type::PalMask);
		image.fillAlpha(255);

		// Read raw pixel data
		data.seek(0, SEEK_SET);
		data.read(imageData(image), info.height * info.width);

		// Convert from column-major to row-major
		image.rotate(90);
		image.mirror(false);

		return true;
	}
};
