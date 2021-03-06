#ifndef _HDF5WRAPPER_H
#define _HDF5WRAPPER_H

#include <vector>
#include <cmath>
#include <iostream>
#include <chrono>
#include <sstream>
#include <iterator>
#include <cstring>
#include <string>
#include <hdf5.h>

#ifdef USEMPI
#include <mpi.h>
#endif

// Overloaded function to return HDF5 type given a C type
static inline hid_t hdf5_type(float dummy)              {return H5T_NATIVE_FLOAT;}
static inline hid_t hdf5_type(double dummy)             {return H5T_NATIVE_DOUBLE;}
static inline hid_t hdf5_type(short dummy)              {return H5T_NATIVE_SHORT;}
static inline hid_t hdf5_type(int dummy)                {return H5T_NATIVE_INT;}
static inline hid_t hdf5_type(long dummy)               {return H5T_NATIVE_LONG;}
static inline hid_t hdf5_type(long long dummy)          {return H5T_NATIVE_LLONG;}
static inline hid_t hdf5_type(unsigned short dummy)     {return H5T_NATIVE_USHORT;}
static inline hid_t hdf5_type(unsigned int dummy)       {return H5T_NATIVE_UINT;}
static inline hid_t hdf5_type(unsigned long dummy)      {return H5T_NATIVE_ULONG;}
static inline hid_t hdf5_type(unsigned long long dummy) {return H5T_NATIVE_ULLONG;}
static inline hid_t hdf5_type(std::string dummy)        {return H5T_C_S1;}
static inline hid_t hdf5_type_from_string(std::string dummy)
{
    if (dummy == std::string("float32")) return H5T_NATIVE_FLOAT;
    else if (dummy == std::string("float64")) return H5T_NATIVE_DOUBLE;
    else if (dummy == std::string("int16")) return H5T_NATIVE_SHORT;
    else if (dummy == std::string("int32")) return H5T_NATIVE_INT;
    else if (dummy == std::string("int64")) return H5T_NATIVE_LLONG;
    else if (dummy == std::string("uint16")) return H5T_NATIVE_USHORT;
    else if (dummy == std::string("uint32")) return H5T_NATIVE_UINT;
    else if (dummy == std::string("uint64")) return H5T_NATIVE_ULLONG;
    else return H5T_C_S1;
}

#if H5_VERSION_GE(1,10,1)
#endif

#if H5_VERSION_GE(1,12,0)
#endif

///\name HDF class to manage writing information
///\todo need to look into whether one can open directly with
/// full path or must open groups explicitly. If latter, updated needed
class H5OutputFile
{

private:

    hid_t file_id;
#ifdef USEPARALLELHDF
    hid_t parallel_access_id;
#endif

protected:

    /// size of chunks when compressing
    unsigned int HDFOUTPUTCHUNKSIZE = 8192;

    /// Called if a HDF5 call fails (might need to MPI_Abort)
    void io_error(std::string message) {
        std::cerr << message << std::endl;
#ifdef USEMPI
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
        abort();
    }

    /// parallel hdf5 mpi routines
#ifdef USEPARALLELHDF
    /// set the offset points for the mpi write
    void _set_mpi_dim_and_offset(MPI_Comm &comm,
        hsize_t rank, std::vector<hsize_t> &dims,
        std::vector<unsigned long long> &dims_single,
        std::vector<unsigned long long> &dims_offset,
        std::vector<unsigned long long> &mpi_hdf_dims,
        std::vector<unsigned long long> &mpi_hdf_dims_tot,
        bool flag_parallel, bool flag_first_dim_parallel
    );
    /// select the hyperslab
    void _set_mpi_hyperslab(hid_t &dspace_id, hid_t &memspace_id,
        hsize_t rank, std::vector<hsize_t> &dims,
        std::vector<unsigned long long> &mpi_hdf_dims_tot,
        bool flag_parallel, bool flag_hyperslab);
    /// and set the transfer properties
    void _set_mpi_dataset_properties(hid_t &prop_id, bool &iwrite,
        hid_t &dspace_id, hid_t &memspace_id,
        hsize_t rank, std::vector<hsize_t> dims, std::vector<hsize_t> dims_offset,
        bool flag_parallel, bool flag_collective, bool flag_hyperslab)
#endif

    /// set chunks size for a dataset
    void _set_chunks(std::vector<hsize_t> &chunks,
        int rank, hsize_t *dims,
    #ifdef USEPARALLELHDF
        std::vector<hsize_t> &mpi_hdf_dims_tot,
    #endif
        bool flag_parallel
    );
    void _set_chunks(std::vector<hsize_t> &chunks,
        std::vector<hsize_t> &dims,
#ifdef USEPARALLELHDF
        std::vector<hsize_t> &mpi_hdf_dims_tot,
#endif
        bool flag_parallel
    )
    {
        _set_chunks(chunks, dims.size(), dims.data(),
#ifdef USEPARALLELHDF
            mpi_hdf_dims_tot,
#endif
            flag_parallel
        );
    }
    hid_t _set_compression(int rank, std::vector<hsize_t> &chunks);

    /// tokenize a path given an input string
    std::vector<std::string> _tokenize(const std::string &s);

    /// get attribute id
    void _get_attribute(std::vector<hid_t> &ids, const std::string attr_name);
    /// get attribute id from tokenized string
    void _get_attribute(std::vector<hid_t> &ids, const std::vector<std::string> &parts);

    /// wrapper for reading scalar
    template<typename T> void _do_read(const hid_t &attr, const hid_t &type, T &val)
    {
        if (hdf5_type(T{}) == H5T_C_S1)
        {
          _do_read_string(attr, type, val);
        }
        else {
          H5Aread(attr, type, &val);
        }
    }
    /// wrapper for reading string
    void _do_read_string(const hid_t &attr, const hid_t &type, std::string &val);
    /// wrapper for reading vector
    template<typename T> void _do_read_v(const hid_t &attr, const hid_t &type, std::vector<T> &val)
    {
        hid_t space = H5Aget_space (attr);
        int npoints = H5Sget_simple_extent_npoints(space);
        val.resize(npoints);
        H5Aread(attr, type, val.data());
        H5Sclose(space);
    }

    /// get dataset id
    void _get_dataset(std::vector<hid_t> &ids, const std::string dset_name);
    /// get dataset id from tokenized string
    void _get_dataset(std::vector<hid_t> &ids, const std::vector<std::string> &parts);

    /// get dataset id
    void _get_hdf5_id(std::vector<hid_t> &ids, const std::string name);
    /// get attribute id from tokenized string
    void _get_hdf5_id(std::vector<hid_t> &ids, const std::vector<std::string> &parts);

public:

    // Constructor
    H5OutputFile();
    // Destructor closes the file if it's open
    ~H5OutputFile();

    /// Create a new file
    void create(std::string filename, hid_t flag = H5F_ACC_TRUNC,
        int taskID = -1, bool iparallelopen = true);
    /// Append to a file
    void append(std::string filename, hid_t flag = H5F_ACC_RDWR,
        int taskID = -1, bool iparallelopen = true);

    /// Close the file
    void close();

    /// create a group
    hid_t create_group(std::string groupname) {
        hid_t group_id;
        if (H5Lexists(group_id, groupname.c_str(), H5P_DEFAULT)) {
            throw std::invalid_argument("Group "+groupname+"already present, not creating group");
        }
        group_id = H5Gcreate(file_id, groupname.c_str(),
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        return group_id;
    }
    hid_t open_group(std::string groupname) {
        hid_t group_id = H5Gopen(file_id, groupname.c_str(), H5P_DEFAULT);
        return group_id;
    }
    /// close group
    herr_t close_group(hid_t gid) {
        herr_t status = H5Gclose(gid);
        return status;
    }

    /// close path
    void close_path(std::string path);
    /// close hids stored in vector
    void close_hdf_ids(std::vector<hid_t> &ids);

    /// create a dataspace
    hid_t create_dataspace(const std::vector<hsize_t> &dims)
    {
        hid_t dspace_id;
        int rank = dims.size();
        dspace_id = H5Screate_simple(rank, dims.data(), NULL);
        return dspace_id;
    }
    hid_t create_dataspace(int rank, hsize_t *dims)
    {
        hid_t dspace_id;
        dspace_id = H5Screate_simple(rank, dims, NULL);
        return dspace_id;
    }
    hid_t create_dataspace(hsize_t len)
    {
        int rank = 1;
        hsize_t dims[1] = {len};
        hid_t dspace_id = H5Screate_simple(rank, dims, NULL);
        return dspace_id;
    }
    /// close data space
    herr_t close_dataspace(hid_t dspace_id) {
        herr_t status = H5Sclose(dspace_id);
        return status;
    }

    /// create a dataset
    hid_t create_dataset(std::string dsetname, hid_t type_id, hid_t dspace_id)
    {
        hid_t dset_id;
        dset_id = H5Dcreate(file_id, dsetname.c_str(), type_id, dspace_id,
          H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        return dset_id;
    }
    template <typename T> hid_t create_dataset(std::string dsetname, T *data, hid_t dspace_id)
    {
        hid_t dset_id;
        hid_t type_id = hdf5_type(T{});
        dset_id = H5Dcreate(file_id, dsetname.c_str(), type_id, dspace_id,
          H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        return dset_id;
    }
    /// create data set and return hid
    /// and possibly close all groups, data spaces and data set itself after creation
    template <typename T> hid_t create_dataset(std::string path, std::string name, T&data,
        std::vector<hsize_t> dims, std::vector<hsize_t> chunkDims = std::vector<hsize_t>(0),
        bool flag_closedataset = true,
        bool flag_parallel = true, bool flag_hyperslab = true, bool flag_collective = true)
    {
        return create_dataset(path+"/"+name, hdf5_type(T{}), dims, chunkDims,
            flag_closedataset,
            flag_parallel, flag_hyperslab, flag_collective);
    }
    hid_t create_dataset(std::string path, std::string name, hid_t datatype,
        std::vector<hsize_t> dims, std::vector<hsize_t> chunkDims = std::vector<hsize_t>(0),
        bool flag_closedataset = true,
        bool flag_parallel = true, bool flag_hyperslab = true, bool flag_collective = true)
    {
        return create_dataset(path+"/"+name, datatype, dims, chunkDims,
            flag_closedataset,
            flag_parallel, flag_hyperslab, flag_collective);
    }
    template <typename T> hid_t create_dataset(std::string fullname, T&data,
        std::vector<hsize_t> dims, std::vector<hsize_t> chunkDims = std::vector<hsize_t>(0),
        bool flag_closedataset = true,
        bool flag_parallel = true, bool flag_hyperslab = true, bool flag_collective = true)
    {
        return create_dataset(fullname, hdf5_type(T{}), dims, chunkDims,
            flag_closedataset,
            flag_parallel, flag_hyperslab, flag_collective);
    }
    hid_t create_dataset(std::string fullname, hid_t datatype,
      std::vector<hsize_t> dims, std::vector<hsize_t> chunkDims = std::vector<hsize_t>(0),
      bool flag_closedataset = true,
      bool flag_parallel = true, bool flag_hyperslab = true, bool flag_collective = true);

    /// close data set
    herr_t close_dataset(hid_t dset_id) {
        herr_t status = H5Dclose(dset_id);
        return status;
    }

    /// find and hdf5 object
    void get_hdf5_id(std::vector<hid_t> &ids, const std::string &name);
    hid_t get_hdf5_id(std::string path, std::string name, bool closeids = true);
    hid_t get_hdf5_id(std::string fullname, bool closeids = true);

    /// create a link
    ///\todo could provide HDF 1.12 viable APIs for external links handling of VOL
    herr_t create_link(std::string orgname, std::string linkname, bool ihard = true);

    /// writes to an existing data set
    /// with a hyperslab selection defined by count, start
    void write_to_dataset(std::string name, hsize_t len, void *data,
        const std::vector<hsize_t>& count, const std::vector<hsize_t>& start,
        hid_t memtype_id=-1, hid_t filetype_id=-1,
        bool flag_parallel = true, bool flag_first_dim_parallel = true,
        bool flag_hyperslab = true, bool flag_collective = true);
    template <typename T> void write_to_dataset(std::string name, hsize_t len, T *data,
        const std::vector<hsize_t>& count, const std::vector<hsize_t>& start,
        hid_t memtype_id = -1, hid_t filetype_id=-1,
        bool flag_parallel = true, bool flag_hyperslab = true, bool flag_collective = true)
    {
        int rank = 1;
        hsize_t dims[1] = {len};
        if (memtype_id == -1) memtype_id = hdf5_type(T{});
        write_to_dataset_nd(name, rank, dims, data,
            count, start,
            memtype_id, filetype_id, flag_parallel, flag_hyperslab, flag_collective);
    }
    /// Write a multidimensional dataset. Data type of the new dataset is taken to be the type of
    /// the input data if not explicitly specified with the filetype_id parameter.
    template <typename T> void write_to_dataset_nd(std::string name, int rank, hsize_t *dims, T *data,
        const std::vector<hsize_t>& count, const std::vector<hsize_t>& start,
        hid_t memtype_id = -1, hid_t filetype_id = -1,
        bool flag_parallel = true, bool flag_first_dim_parallel = true,
        bool flag_hyperslab = true, bool flag_collective = true)
    {
        memtype_id = hdf5_type(T{});
        write_to_dataset_nd(name, rank, dims, (void*)data,
            count, start,
            memtype_id, filetype_id,
            flag_parallel, flag_first_dim_parallel,
            flag_hyperslab, flag_collective);
    }
    template <typename T> void write_to_dataset_nd(std::string name, std::vector<hsize_t> dims, T *data,
        const std::vector<hsize_t>& count, const std::vector<hsize_t>& start,
        hid_t memtype_id = -1, hid_t filetype_id = -1,
        bool flag_parallel = true, bool flag_first_dim_parallel = true,
        bool flag_hyperslab = true, bool flag_collective = true)
    {
        write_to_dataset_nd(name, dims.size(), dims.data(),(void*)data,
            count, start,
            memtype_id, filetype_id,
            flag_parallel, flag_first_dim_parallel,
            flag_hyperslab, flag_collective);
    }
    //write dataset with hyperslab selection
    void write_to_dataset_nd(std::string name, int rank, hsize_t *dims, void *data,
        const std::vector<hsize_t>& count, const std::vector<hsize_t>& start,
        hid_t memtype_id = -1, hid_t filetype_id=-1,
        bool flag_parallel = true, bool flag_first_dim_parallel = true,
        bool flag_hyperslab = true, bool flag_collective = true);

    /// write 1D data sets of string or input data with defined data type
    /// without hyperslab selection, creates dataset, writes to it and closes
    void write_dataset(std::string name, hsize_t len, std::string data,
        bool flag_parallel = true, bool flag_collective = true);
    void write_dataset(std::string name, hsize_t len, void *data,
        hid_t memtype_id=-1, hid_t filetype_id=-1,
        bool flag_parallel = true, bool flag_first_dim_parallel = true,
        bool flag_hyperslab = true, bool flag_collective = true);
    /// Write a new 1D dataset. Data type of the new dataset is taken to be the type of
    /// the input data if not explicitly specified with the filetype_id parameter.
    /// template function so defined here
    template <typename T> void write_dataset(std::string name, hsize_t len, T *data,
        hid_t memtype_id = -1, hid_t filetype_id=-1,
        bool flag_parallel = true, bool flag_hyperslab = true, bool flag_collective = true)
    {
        int rank = 1;
        hsize_t dims[1] = {len};
        if (memtype_id == -1) memtype_id = hdf5_type(T{});
        write_dataset_nd(name, rank, dims, data, memtype_id, filetype_id, flag_parallel, flag_hyperslab, flag_collective);
    }
    /// Write a multidimensional dataset. Data type of the new dataset is taken to be the type of
    /// the input data if not explicitly specified with the filetype_id parameter.
    template <typename T> void write_dataset_nd(std::string name, int rank, hsize_t *dims, T *data,
        hid_t memtype_id = -1, hid_t filetype_id = -1,
        bool flag_parallel = true, bool flag_first_dim_parallel = true,
        bool flag_hyperslab = true, bool flag_collective = true)
    {
        memtype_id = hdf5_type(T{});
        write_dataset_nd(name, rank, dims, (void*)data,
            memtype_id, filetype_id,
            flag_parallel, flag_first_dim_parallel,
            flag_hyperslab, flag_collective);
// #ifdef USEPARALLELHDF
//         MPI_Comm comm = mpi_comm_write;
//         MPI_Info info = MPI_INFO_NULL;
// #endif
//         hid_t dspace_id, dset_id, prop_id, memspace_id, ret;
//         std::vector<hsize_t> chunks;
//
//         // Get HDF5 data type of the array in memory
//         if (memtype_id == -1) memtype_id = hdf5_type(T{});
//
//         // Determine type of the dataset to create
//         if(filetype_id < 0) filetype_id = memtype_id;
//
// #ifdef USEPARALLELHDF
//         std::vector<unsigned long long> mpi_hdf_dims(rank*NProcsWrite), mpi_hdf_dims_tot(rank), dims_single(rank), dims_offset(rank);
//         _set_mpi_dim_and_offset(comm, rank, dims, dims_single, dims_offset, mpi_hdf_dims, mpi_hdf_dims_tot, flag_parallel, flag_first_dim_parallel);
// #endif
//
//         // Determine if going to compress data in chunks
//         // Only chunk non-zero size datasets
//         _set_chunks(chunks, rank, dims,
// #ifdef USEPARALLELHDF
//             mpi_hdf_dims_tot,
// #endif
//             flag_parallel
//         );
//
//         // Create the dataspace
// #ifdef USEPARALLELHDF
//         _set_mpi_hyperslab(dspace_id, memspace_id, rank, dims, mpi_hdf_dims_tot, flag_parallel, flag_hyperslab);
// #else
//         dspace_id = H5Screate_simple(rank, dims, NULL);
//         memspace_id = dspace_id;
// #endif
//
//         // Dataset creation properties
//         prop_id = H5P_DEFAULT;
// #ifdef USEHDFCOMPRESSION
//         prop_id = _set_compression(rank, chunks);
// #endif
//         // Create the dataset
//         dset_id = H5Dcreate(file_id, name.c_str(), filetype_id, dspace_id,
//             H5P_DEFAULT, prop_id, H5P_DEFAULT);
//         if(dset_id < 0) io_error(std::string("Failed to create dataset: ")+name);
//         H5Pclose(prop_id);
//
//         prop_id = H5P_DEFAULT;
//         bool iwrite = (dims[0] > 0);
// #ifdef USEPARALLELHDF
//         _set_mpi_dataset_properties(prop_id, iwrite,
//             dspace_id, memspace_id,
//             rank, dims, dims_offset,
//             flag_parallel, flag_collective, flag_hyperslab);
// #endif
//         if (iwrite) {
//             ret = H5Dwrite(dset_id, memtype_id, memspace_id, dspace_id, prop_id, data);
//             if (ret < 0) io_error(std::string("Failed to write dataset: ")+name);
//         }
//         // Clean up (note that dtype_id is NOT a new object so don't need to close it)
//         H5Pclose(prop_id);
// #ifdef USEPARALLELHDF
//         if (flag_hyperslab && flag_parallel) H5Sclose(memspace_id);
// #endif
//         H5Sclose(dspace_id);
//         H5Dclose(dset_id);
    }
    template <typename T> void write_dataset_nd(std::string name, std::vector<hsize_t> dims, T *data,
        hid_t memtype_id = -1, hid_t filetype_id = -1,
        bool flag_parallel = true, bool flag_first_dim_parallel = true,
        bool flag_hyperslab = true, bool flag_collective = true)
    {
        write_dataset_nd(name, dims.size(), dims.data(), data,
            memtype_id, filetype_id,
            flag_parallel, flag_first_dim_parallel,
            flag_hyperslab, flag_collective);
    }
    //write dataset with hyperslab selection
    void write_dataset_nd(std::string name, int rank, hsize_t *dims, void *data,
        hid_t memtype_id = -1, hid_t filetype_id=-1,
        bool flag_parallel = true, bool flag_first_dim_parallel = true,
        bool flag_hyperslab = true, bool flag_collective = true);

    /// with a hyperslab selection defined by count, start
    void write_dataset(std::string name, hsize_t len, std::string data,
        const std::vector<hsize_t>& count, const std::vector<hsize_t>& start,
        bool flag_parallel = true, bool flag_collective = true);
    void write_dataset(std::string name, hsize_t len, void *data,
        const std::vector<hsize_t>& count, const std::vector<hsize_t>& start,
        hid_t memtype_id=-1, hid_t filetype_id=-1,
        bool flag_parallel = true, bool flag_first_dim_parallel = true,
        bool flag_hyperslab = true, bool flag_collective = true);
    /// Write a new 1D dataset. Data type of the new dataset is taken to be the type of
    /// the input data if not explicitly specified with the filetype_id parameter.
    /// template function so defined here
    template <typename T> void write_dataset(std::string name, hsize_t len, T *data,
        const std::vector<hsize_t>& count, const std::vector<hsize_t>& start,
        hid_t memtype_id = -1, hid_t filetype_id=-1,
        bool flag_parallel = true, bool flag_hyperslab = true, bool flag_collective = true)
    {
        int rank = 1;
        hsize_t dims[1] = {len};
        if (memtype_id == -1) memtype_id = hdf5_type(T{});
        write_dataset_nd(name, rank, dims, data,
            count, start, memtype_id, filetype_id,
            flag_parallel, flag_hyperslab, flag_collective);
    }

    /// Write a multidimensional dataset. Data type of the new dataset is taken to be the type of
    /// the input data if not explicitly specified with the filetype_id parameter.
    template <typename T> void write_dataset_nd(std::string name, std::vector<hsize_t> dims, T *data,
        const std::vector<hsize_t>& count, const std::vector<hsize_t>& start,
        hid_t memtype_id = -1, hid_t filetype_id = -1,
        bool flag_parallel = true, bool flag_first_dim_parallel = true,
        bool flag_hyperslab = true, bool flag_collective = true)
    {
        write_dataset_nd(name, dims.size(), dims.data(), data,
            count, start,
            memtype_id, filetype_id,
            flag_parallel, flag_first_dim_parallel,
            flag_hyperslab, flag_collective);
    }
    template <typename T> void write_dataset_nd(std::string name, int rank, hsize_t *dims, T *data,
        const std::vector<hsize_t>& count, const std::vector<hsize_t>& start,
        hid_t memtype_id = -1, hid_t filetype_id = -1,
        bool flag_parallel = true, bool flag_first_dim_parallel = true,
        bool flag_hyperslab = true, bool flag_collective = true)
    {
#ifdef USEPARALLELHDF
    MPI_Comm comm = mpi_comm_write;
    MPI_Info info = MPI_INFO_NULL;
#endif
    hid_t dspace_id, dset_id, prop_id, memspace_id, ret;
    std::vector<hsize_t> chunks(rank);

    // Get HDF5 data type of the array in memory
    if (memtype_id == -1) memtype_id = hdf5_type(T{});

    // Determine type of the dataset to create
    if(filetype_id < 0) filetype_id = memtype_id;

#ifdef USEPARALLELHDF
    std::vector<unsigned long long> mpi_hdf_dims(rank*NProcsWrite), mpi_hdf_dims_tot(rank), dims_single(rank), dims_offset(rank);
    // _set_mpi_dim_and_offset(comm, rank, dims, dims_single, dims_offset, mpi_hdf_dims, mpi_hdf_dims_tot, flag_parallel, flag_first_dim_parallel);
#endif

    // Determine if going to compress data in chunks
    // Only chunk non-zero size datasets
    _set_chunks(chunks, rank, dims,
#ifdef USEPARALLELHDF
        mpi_hdf_dims_tot,
#endif
        flag_parallel
    );

    // Create the dataspace
#ifdef USEPARALLELHDF
    // _set_mpi_hyperslab(dspace_id, memspace_id, rank, dims, mpi_hdf_dims_tot, flag_parallel, flag_hyperslab);
#else
    dspace_id = H5Screate_simple(rank, dims, NULL);
    memspace_id = dspace_id;
#endif

    // Dataset creation properties
    prop_id = H5P_DEFAULT;
#ifdef USEHDFCOMPRESSION
    prop_id = _set_compression(rank, chunks);
#endif
    // Create the dataset
    dset_id = H5Dcreate(file_id, name.c_str(), filetype_id, dspace_id,
        H5P_DEFAULT, prop_id, H5P_DEFAULT);
    if(dset_id < 0) io_error(std::string("Failed to create dataset: ")+name);
    H5Pclose(prop_id);

    prop_id = H5P_DEFAULT;
    bool iwrite = (dims[0] > 0);
    if (!count.empty() && !start.empty()) {
        herr_t ret = H5Sselect_hyperslab(dspace_id, H5S_SELECT_SET, start.data(), NULL, count.data(), NULL);
    }
#ifdef USEPARALLELHDF
    // _set_mpi_dataset_properties(prop_id, iwrite,
    //     dspace_id, memspace_id,
    //     rank, dims, dims_offset,
    //     flag_parallel, flag_collective, flag_hyperslab);
#endif
    if (iwrite) {
        ret = H5Dwrite(dset_id, memtype_id, memspace_id, dspace_id, prop_id, data);
        if (ret < 0) io_error(std::string("Failed to write dataset: ")+name);
    }

    // Clean up (note that dtype_id is NOT a new object so don't need to close it)
    H5Pclose(prop_id);
#ifdef USEPARALLELHDF
    if (flag_hyperslab && flag_parallel) H5Sclose(memspace_id);
#endif
    H5Sclose(dspace_id);
    H5Dclose(dset_id);
    }

    void write_dataset_nd(std::string name, int rank, hsize_t *dims, void *data,
        const std::vector<hsize_t>& count, const std::vector<hsize_t>& start,
        hid_t memtype_id = -1, hid_t filetype_id=-1,
        bool flag_parallel = true, bool flag_first_dim_parallel = true,
        bool flag_hyperslab = true, bool flag_collective = true);

    /// get a dataset with full path given by name
    void get_dataset(std::vector<hid_t> &ids, const std::string &name);
    /// check if dataset exits
    bool exists_dataset(const std::string &parent, const std::string &name);

    /// get an attribute with full path given by name
    void get_attribute(std::vector<hid_t> &ids, const std::string &name);

    /// read scalar attribute
    template<typename T> const T read_attribute(const std::string &name)
    {
        std::string attr_name;
        T val;
        hid_t type;
        H5O_info_t object_info;
        std::vector <hid_t> ids;
        //traverse the file to get to the attribute, storing the ids of the
        //groups, data spaces, etc that have been opened.
        get_attribute(ids, name);
        //now reverse ids and load attribute
        reverse(ids.begin(),ids.end());
        //determine hdf5 type of the array in memory
        type = hdf5_type(T{});
        // read the data
        _do_read<T>(ids[0], type, val);
        H5Aclose(ids[0]);
        ids.erase(ids.begin());
        //now have hdf5 ids traversed to get to desired attribute so move along to close all
        //based on their object type
        close_hdf_ids(ids);
        return val;
    }
    /// read vector attribute
    template<typename T> const std::vector<T> read_attribute_v(const std::string &name)
    {
        std::string attr_name;
        std::vector<T> val;
        hid_t type;
        H5O_info_t object_info;
        std::vector <hid_t> ids;
        //traverse the file to get to the attribute, storing the ids of the
        //groups, data spaces, etc that have been opened.
        get_attribute(ids, name);
        //now reverse ids and load attribute
        reverse(ids.begin(),ids.end());
        //determine hdf5 type of the array in memory
        type = hdf5_type(T{});
        // read the data
        _do_read_v<T>(ids[0], type, val);
        H5Aclose(ids[0]);
        ids.erase(ids.begin());
        //now have hdf5 ids traversed to get to desired attribute so move along to close all
        //based on their object type
        close_hdf_ids(ids);
        return val;
    }

    /// sees if attribute exits
    bool exists_attribute(const std::string &parent, const std::string &name);

    /// write an attribute, not that since these are template function, define here in the header
    template <typename T> void write_attribute(const std::string &parent, const std::string &name, const std::vector<T> &data)
    {
        // Get HDF5 data type of the value to write
        hid_t dtype_id = hdf5_type(data[0]);
        hsize_t size = data.size();

        // Open the parent object
        hid_t parent_id = H5Oopen(file_id, parent.c_str(), H5P_DEFAULT);
        if(parent_id < 0)io_error(std::string("Unable to open object to write attribute: ")+name);

        // Create dataspace
        hid_t dspace_id = H5Screate(H5S_SIMPLE);
        hid_t dspace_extent  = H5Sset_extent_simple(dspace_id, 1, &size, NULL);

        // Create attribute
        hid_t attr_id = H5Acreate(parent_id, name.c_str(), dtype_id, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        if(attr_id < 0)io_error(std::string("Unable to create attribute ")+name+std::string(" on object ")+parent);

        // Write the attribute
        if(H5Awrite(attr_id, dtype_id, data.data()) < 0)
        io_error(std::string("Unable to write attribute ")+name+std::string(" on object ")+parent);

        // Clean up
        H5Aclose(attr_id);
        H5Sclose(dspace_id);
        H5Oclose(parent_id);
    }

    template <typename T> void write_attribute(const std::string &parent, const std::string &name, const T &data)
    {
        // Get HDF5 data type of the value to write
        hid_t dtype_id = hdf5_type(data);

        // Open the parent object
        hid_t parent_id = H5Oopen(file_id, parent.c_str(), H5P_DEFAULT);
        if(parent_id < 0)io_error(std::string("Unable to open object to write attribute: ")+name);

        // Create dataspace
        hid_t dspace_id = H5Screate(H5S_SCALAR);

        // Create attribute
        hid_t attr_id = H5Acreate(parent_id, name.c_str(), dtype_id, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        if(attr_id < 0)io_error(std::string("Unable to create attribute ")+name+std::string(" on object ")+parent);

        // Write the attribute
        if(H5Awrite(attr_id, dtype_id, &data) < 0)
        io_error(std::string("Unable to write attribute ")+name+std::string(" on object ")+parent);

        // Clean up
        H5Aclose(attr_id);
        H5Sclose(dspace_id);
        H5Oclose(parent_id);
    }

    void write_attribute(const std::string parent, const std::string &name, std::string data)
    {
        // Get HDF5 data type of the value to write
        hid_t dtype_id = H5Tcopy(H5T_C_S1);
        if (data.size() == 0) data=" ";
        H5Tset_size(dtype_id, data.size());
        H5Tset_strpad(dtype_id, H5T_STR_NULLTERM);

        // Open the parent object
        hid_t parent_id = H5Oopen(file_id, parent.c_str(), H5P_DEFAULT);
        if(parent_id < 0)io_error(std::string("Unable to open object to write attribute: ")+name);

        // Create dataspace
        hid_t dspace_id = H5Screate(H5S_SCALAR);

        // Create attribute
        hid_t attr_id = H5Acreate(parent_id, name.c_str(), dtype_id, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        if(attr_id < 0)io_error(std::string("Unable to create attribute ")+name+std::string(" on object ")+parent);

        // Write the attribute
        if(H5Awrite(attr_id, dtype_id, data.c_str()) < 0)
        io_error(std::string("Unable to write attribute ")+name+std::string(" on object ")+parent);

        // Clean up
        H5Aclose(attr_id);
        H5Sclose(dspace_id);
        H5Oclose(parent_id);
    }

};

#endif
