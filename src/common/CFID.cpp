#include <fstream>
#include <boost/tokenizer.hpp>
#include "CFID.hpp"
#include "cvm_util.hpp"
#include "Options.hpp"
#include "fidio/CDICOMFile.hpp"
#include "fidio/CFIDReaderDPT.hpp"
#include "fidio/CFIDReaderSiemens.hpp"
#include "fidio/CFIDReaderDCM.hpp"
#include "fidio/CFIDReaderRDA.hpp"
#include "fidio/CFIDReaderLCM.hpp"
#include "fidio/CFIDReaderPhilips.hpp"
#include "fidio/CFIDReaderPhilipsDCM.hpp"
#include "fidio/CFIDReaderGE.hpp"
#include "fidio/CFIDReaderVarian.hpp"
#include "fidio/CFIDReaderBruker.hpp"
#include "fidio/CFIDReaderJMRUITXT.hpp"
#include "preprocess.hpp"
#include "Workspace.hpp"

//! Constructor.
tarquin::CFID::CFID() : 
	m_sampling_frequency(0.0, false), 
	m_transmitter_frequency(0.0, false), 
	m_pulse_sequence("press", false),
	m_num_averages(0, false),
	m_echo_time(0.0, false), 
	m_nPoints(0), 
	m_norm_val(1.0),
    m_wref(false),
    m_dyn(false),
    m_cwf(false),
	m_rows(1),
	m_cols(1),
	m_slices(1)
{
	ClearVectors();

    std::vector<double> zero;
    zero.assign(3,0);
    m_voxel_dim = std::make_pair(zero, false);
    m_voi_dim = std::make_pair(zero, false);
    m_pos = std::make_pair(zero, false);
    m_row_dirn = std::make_pair(zero, false);
    m_col_dirn = std::make_pair(zero, false);
}

//! Destructor.
tarquin::CFID::~CFID()
{

}

void tarquin::CFID::ShiftGrid(double row_shift, double col_shift, double slice_shift, CBoswell& log)
{
    log.LogMessage(LOG_INFO, "Starting grid shift.");
    // check that row, col and slice are between -1 and +1
    if ( ( abs(row_shift) > 1 ) || ( abs(col_shift) > 1 ) || ( abs(slice_shift) > 1 ) )
        log.LogMessage(LOG_WARNING, "Grid shift of greater than one voxel requested.");
    // check the fid is of the appropriate dimensions
    if ( m_rows < 2 && row_shift != 0 )
        log.LogMessage(LOG_WARNING, "Row shift cannot be applied to this data.");
    if ( m_cols < 2 && col_shift != 0 )
        log.LogMessage(LOG_WARNING, "Col shift cannot be applied to this data.");
    if ( m_slices < 2 && slice_shift != 0 )
        log.LogMessage(LOG_WARNING, "Slice shift cannot be applied to this data.");

    // convert FID array into k-space planes, one per time point,
    // by performing the 2D fft
    cmat_stdvec kspace_stack;
    for ( int time_pt = 0; time_pt < m_nPoints; time_pt++ )
    {
        int n = 1;
        n = n * 2;
    }

    cvec_stdvec m_cvmFID;

    // apply the shift in k-space

    // transform back to the spatial domain

    // over-write the FID array
}

void tarquin::CFID::trans_kspace(Options& options, CBoswell& log)
{
    log.LogMessage(LOG_INFO, "Transforming to k-space");

    // convert zfilled k-space back to fid list
    cvec_stdvec new_fid;

    for ( size_t slice = 1; slice < ( m_slices + 1 ); slice++ )
    {

        // convert FID array into k-space planes, one per time point,
        // by performing the 2D fft
        cmat_stdvec kspace;
        for ( size_t time_pt = 1; time_pt < ( m_nPoints + 1 ); time_pt++ )
        {
            //std::cout << std::endl << m_rows << " " << m_cols << std::endl;
            cvm::cmatrix temp_mat(m_rows, m_cols);
            for ( size_t row = 1; row < ( m_rows + 1 ); row++ )
            {
                for ( size_t col = 1; col < ( m_cols + 1 ); col++ )
                {

                    coord voxel(row, col, slice); 
                    cvm::cvector& temp_fid = GetVectorFID(voxel);
                    temp_mat(row, col) = temp_fid(time_pt);
                }
            }

            // 2D ifft on temp_mat
            temp_mat = ifft(temp_mat);
            temp_mat = fftshift(temp_mat);
            // transpose
            cvm::cmatrix temp_mat_trans = ~temp_mat;
            cvm::cmatrix temp_mat_conj(temp_mat_trans.real(),-temp_mat_trans.imag());
            temp_mat_trans = temp_mat_conj;
            temp_mat_trans = ifft(temp_mat_trans);
            temp_mat_trans = fftshift(temp_mat_trans);
            // transpose back
            temp_mat = ~temp_mat_trans;
            cvm::cmatrix temp_mat_conj2(temp_mat.real(),-temp_mat.imag());
            temp_mat = temp_mat_conj2;

            kspace.push_back(temp_mat);
        }


        cvm::cmatrix temp_mat(m_rows, m_cols);
        for ( size_t col = 1; col < ( m_cols + 1 ); col++ )
        {
            for ( size_t row = 1; row < ( m_rows + 1 ); row++ )
            {
                cvm::cvector temp_fid(m_nPoints);
                for ( size_t time_pt = 1; time_pt < ( m_nPoints + 1 ); time_pt++ )
                {
                    temp_fid(time_pt) = kspace[time_pt-1](row,col);
                }
                new_fid.push_back(temp_fid);
            }
        }
    }

    m_cvmFID = new_fid;
}

void tarquin::CFID::filter_kspace(Options& options, CBoswell& log)
{

    log.LogMessage(LOG_INFO, "Filtering k-space");


    cvec_stdvec new_fid;
    for ( size_t slice = 1; slice < ( m_slices + 1 ); slice++ )
    {

        // convert FID array into k-space planes, one per time point,
        // by performing the 2D fft
        cmat_stdvec filter_kspace;
        for ( size_t time_pt = 1; time_pt < ( m_nPoints + 1 ); time_pt++ )
        {
            //std::cout << std::endl << m_rows << " " << m_cols << std::endl;
            cvm::cmatrix temp_mat(m_rows, m_cols);
            for ( size_t row = 1; row < ( m_rows + 1 ); row++ )
            {
                for ( size_t col = 1; col < ( m_cols + 1 ); col++ )
                {

                    coord voxel(row, col, slice); 
                    cvm::cvector& temp_fid = GetVectorFID(voxel);
                    temp_mat(row, col) = temp_fid(time_pt);
                }
            }

            // 2D ifft on temp_mat
            temp_mat = ifft(temp_mat);
            temp_mat = ifftshift(temp_mat);
            // transpose
            cvm::cmatrix temp_mat_trans = ~temp_mat;
            cvm::cmatrix temp_mat_conj(temp_mat_trans.real(),-temp_mat_trans.imag());
            temp_mat_trans = temp_mat_conj;
            temp_mat_trans = ifft(temp_mat_trans);
            temp_mat_trans = ifftshift(temp_mat_trans);
            // transpose back
            temp_mat = ~temp_mat_trans;
            cvm::cmatrix temp_mat_conj2(temp_mat.real(),-temp_mat.imag());
            temp_mat = temp_mat_conj2;
    
            // contstruct row hamming window function
		    cvm::rvector row_wind_fun( m_rows, 0 );
		    for ( int k = 1; k < m_rows + 1; k++ )
                row_wind_fun(k) = 0.54 - 0.46*cos((k-1)*2.0*M_PI / (m_rows-1));

            double row_wind_fun_sum = 0;
            for ( int n = 1; n < m_rows + 1; n++ )
                row_wind_fun_sum += row_wind_fun(n);

            // contstruct col hamming window function
		    cvm::rvector col_wind_fun( m_cols, 0 );
		    for ( int k = 1; k < m_cols + 1; k++ )
                col_wind_fun(k) = 0.54 - 0.46*cos((k-1)*2.0*M_PI / (m_cols-1));

            double col_wind_fun_sum = 0;
            for ( int n = 1; n < m_cols + 1; n++ )
                col_wind_fun_sum += col_wind_fun(n);

            row_wind_fun = row_wind_fun/row_wind_fun_sum;
            col_wind_fun = col_wind_fun/col_wind_fun_sum;
            
            /*
            if ( time_pt == 1 )
            {
                plot(row_wind_fun);
                plot(col_wind_fun);
            }*/

            treal factor_sum = 0;
            cvm::rmatrix filter_mat(m_rows,m_cols);
            for ( size_t row = 1; row < ( m_rows + 1 ); row++ )
                for ( size_t col = 1; col < ( m_cols + 1 ); col++ )
                {
                    treal factor = col_wind_fun(col) * row_wind_fun(row);
                    filter_mat(row,col) = factor;
                    factor_sum = factor_sum + factor;
                }


            // apply filter to temp_mat 
            cvm::cmatrix temp_mat_filter(m_rows,m_cols);
            for ( size_t row = 1; row < ( m_rows + 1 ); row++ )
            {
                for ( size_t col = 1; col < ( m_cols + 1 ); col++ )
                {
                    treal factor = filter_mat(row,col);
                    temp_mat_filter(row,col) = temp_mat(row,col) * factor / factor_sum;
                }
            }

            // 2D fft
            temp_mat_filter = fftshift(temp_mat_filter);
            temp_mat_filter = fft(temp_mat_filter);
            // transpose
            cvm::cmatrix temp_mat_filter_trans = ~temp_mat_filter;
            cvm::cmatrix temp_mat_filter_conj(temp_mat_filter_trans.real(),-temp_mat_filter_trans.imag());
            temp_mat_filter_trans = temp_mat_filter_conj;
            temp_mat_filter_trans = fftshift(temp_mat_filter_trans);
            temp_mat_filter_trans = fft(temp_mat_filter_trans);
            // transpose back
            temp_mat_filter = ~temp_mat_filter_trans;
            cvm::cmatrix temp_mat_filter_conj2(temp_mat_filter.real(),-temp_mat_filter.imag());
            temp_mat_filter = temp_mat_filter_conj2;
            filter_kspace.push_back(temp_mat_filter);
        }


        // convert zfilled k-space back to fid list

        cvm::cmatrix temp_mat(m_rows, m_cols);
        for ( size_t col = 1; col < ( m_cols + 1 ); col++ )
        {
            for ( size_t row = 1; row < ( m_rows + 1 ); row++ )
            {
                cvm::cvector temp_fid(m_nPoints);
                for ( size_t time_pt = 1; time_pt < ( m_nPoints + 1 ); time_pt++ )
                {
                    temp_fid(time_pt) = filter_kspace[time_pt-1](row,col);
                }
                new_fid.push_back(temp_fid);
            }
        }
    }

    m_cvmFID = new_fid;
}

void tarquin::CFID::zfill_kspace(size_t factor, Options& options, CBoswell& log)
{

    log.LogMessage(LOG_INFO, "Z-filling k-space");


    cvec_stdvec new_fid;
    for ( size_t slice = 1; slice < ( m_slices + 1 ); slice++ )
    {

        // convert FID array into k-space planes, one per time point,
        // by performing the 2D fft
        cmat_stdvec zfill_kspace;
        for ( size_t time_pt = 1; time_pt < ( m_nPoints + 1 ); time_pt++ )
        {
            //std::cout << std::endl << m_rows << " " << m_cols << std::endl;
            cvm::cmatrix temp_mat(m_rows, m_cols);
            for ( size_t row = 1; row < ( m_rows + 1 ); row++ )
            {
                for ( size_t col = 1; col < ( m_cols + 1 ); col++ )
                {

                    coord voxel(row, col, slice); 
                    cvm::cvector& temp_fid = GetVectorFID(voxel);
                    temp_mat(row, col) = temp_fid(time_pt);
                }
            }

            // 2D ifft on temp_mat
            temp_mat = ifft(temp_mat);
            temp_mat = ifftshift(temp_mat);
            // transpose
            cvm::cmatrix temp_mat_trans = ~temp_mat;
            cvm::cmatrix temp_mat_conj(temp_mat_trans.real(),-temp_mat_trans.imag());
            temp_mat_trans = temp_mat_conj;
            temp_mat_trans = ifft(temp_mat_trans);
            temp_mat_trans = ifftshift(temp_mat_trans);
            // transpose back
            temp_mat = ~temp_mat_trans;
            cvm::cmatrix temp_mat_conj2(temp_mat.real(),-temp_mat.imag());
            temp_mat = temp_mat_conj2;

            /*cvm::cvector plot_vec1(m_rows);
              plot_vec1 = temp_mat(10);
              plot(plot_vec1);
             */

            // zero-fill
            cvm::cmatrix temp_mat_zfill(m_rows*factor, m_cols*factor);
            int row_start = (int) round(m_rows*factor/2.0-m_rows/2.0);
            int col_start = (int) round(m_cols*factor/2.0-m_cols/2.0);
            
            if ( m_rows%2 == 0 ) // if even
                row_start = row_start + 1;
 
            if ( m_cols%2 == 0 ) // if even
                col_start = col_start + 1;

            temp_mat_zfill.assign(row_start,col_start,temp_mat); // this probobally needs checking for factors != 2

            //cvm::cvector plot_vec2(m_rows);
            //plot_vec2 = temp_mat(8);
            //plot(plot_vec2);

            /*        cvm::cvector plot_vec(m_rows*factor);
                      plot_vec = temp_mat_zfill(16);
                      plot(plot_vec);
             */

            // 2D fft
            temp_mat_zfill = fftshift(temp_mat_zfill);
            temp_mat_zfill = fft(temp_mat_zfill);
            // transpose
            cvm::cmatrix temp_mat_zfill_trans = ~temp_mat_zfill;
            cvm::cmatrix temp_mat_zfill_conj(temp_mat_zfill_trans.real(),-temp_mat_zfill_trans.imag());
            temp_mat_zfill_trans = temp_mat_zfill_conj;
            temp_mat_zfill_trans = fftshift(temp_mat_zfill_trans);
            temp_mat_zfill_trans = fft(temp_mat_zfill_trans);
            // transpose back
            temp_mat_zfill = ~temp_mat_zfill_trans;
            cvm::cmatrix temp_mat_zfill_conj2(temp_mat_zfill.real(),-temp_mat_zfill.imag());
            temp_mat_zfill = temp_mat_zfill_conj2;
            zfill_kspace.push_back(temp_mat_zfill);
        }


        // convert zfilled k-space back to fid list

        cvm::cmatrix temp_mat(m_rows*factor, m_cols*factor);
        for ( size_t col = 1; col < ( m_cols*factor + 1 ); col++ )
        {
            for ( size_t row = 1; row < ( m_rows*factor + 1 ); row++ )
            {
                cvm::cvector temp_fid(m_nPoints);
                for ( size_t time_pt = 1; time_pt < ( m_nPoints + 1 ); time_pt++ )
                {
                    temp_fid(time_pt) = zfill_kspace[time_pt-1](row,col);
                }
                new_fid.push_back(temp_fid);
            }
        }
    }

    m_cvmFID = new_fid;


    // update FID paras
    m_rows = m_rows * factor;
    m_cols = m_cols * factor;
    m_voxel_dim.first[0] = m_voxel_dim.first[0] / factor;
    m_voxel_dim.first[1] = m_voxel_dim.first[1] / factor;

    cvm::rvector cvm_row_ori(3);
    cvm_row_ori(1) = m_row_dirn.first[0];
    cvm_row_ori(2) = m_row_dirn.first[1];
    cvm_row_ori(3) = m_row_dirn.first[2];

    cvm::rvector cvm_col_ori(3);
    cvm_col_ori(1) = m_col_dirn.first[0];
    cvm_col_ori(2) = m_col_dirn.first[1];
    cvm_col_ori(3) = m_col_dirn.first[2];

    cvm::rvector cvm_pos_vec(3);
    cvm_pos_vec(1) = m_pos.first[0];
    cvm_pos_vec(2) = m_pos.first[1];
    cvm_pos_vec(3) = m_pos.first[2];

    cvm::rvector pos_shift(3);
    pos_shift = -cvm_row_ori*m_voxel_dim.first[0]*(factor-1)/2.0 - cvm_col_ori*m_voxel_dim.first[1]*(factor-1)/2.0;

    cvm_pos_vec = cvm_pos_vec + pos_shift;
    m_pos.first[0] = cvm_pos_vec(1);
    m_pos.first[1] = cvm_pos_vec(2);
    m_pos.first[2] = cvm_pos_vec(3);


}

cvm::rvector tarquin::CFID::GetFreqScale() const
{
	int m_N = GetNumberOfPoints();
	cvm::rvector freq_scale(GetNumberOfPoints());

	treal fs = m_sampling_frequency.first;

	// create freq_scale
	for(int n = 1; n<= m_N; n++)
		freq_scale(n) = -1/(2*1/fs)+(n-1)/(1/fs*m_N);

	return freq_scale;
}

cvm::rvector tarquin::CFID::GetFreqScale(int zf) const
{
	int m_N = GetNumberOfPoints()*zf;
	cvm::rvector freq_scale(GetNumberOfPoints()*zf);

	treal fs = m_sampling_frequency.first;

	// create freq_scale
	for(int n = 1; n<= m_N; n++)
		freq_scale(n) = -1/(2*1/fs)+(n-1)/(1/fs*m_N);
	return freq_scale;
}

cvm::rvector tarquin::CFID::GetSpec(int zf)
{
    cvm::cvector fid = m_cvmFID[0]; //TODO
    fid.resize(GetNumberOfPoints()*zf);
	cvm::cvector spec(GetNumberOfPoints()*zf);
    fft(fid, spec);
    spec = fftshift(spec);

	return spec.real();
}


cvm::rvector tarquin::CFID::GetTimeScale() const
{
	assert( GetNumberOfPoints() > 0 );
	assert( m_sampling_frequency.second == true );

	int m_N = GetNumberOfPoints();
	cvm::rvector time_scale(GetNumberOfPoints());
	
	treal fs = m_sampling_frequency.first;
	
	// create freq_scale
	for(int n = 1; n<= m_N; n++)
		time_scale(n) = (n-1)/fs;

	return time_scale;
}

cvm::rvector tarquin::CFID::GetPPMScale(const coord& voxel, int zf) const
{
	int m_N = GetNumberOfPoints()*zf;
	const cvm::rvector freq_scale = GetFreqScale(zf);

	cvm::rvector ppm_scale = freq_scale/m_transmitter_frequency.first*1e6;

	int v = vox2ind(voxel);
	// subtract offset
	for(int n = 1; n<= m_N; n++) {
		ppm_scale(n) = -ppm_scale(n) + m_ref[v].first;
	}
	return ppm_scale;
}

// warning, there seems to be a problem with this function...
/*
bool tarquin::CFID::SaveToBinFile(std::string filename)
{
	std::ofstream fout(filename.c_str(), std::ios_base::binary);

	if( fout.bad() )
		throw std::runtime_error("failed to open output file for writing");

    //plot(m_cvmFID[0]);

	for( int n = 0; n < m_cvmFID[0].size(); ++n )
	{
		treal x = real(m_cvmFID[0][n+1]);
		treal y = imag(m_cvmFID[0][n+1]);

		fout.write((char*)&x, 4);
		fout.write((char*)&y, 4);
	}

	return true;
}*/

// assumes you want the first FID for multi-voxel fitting
bool tarquin::CFID::SaveToFile(std::string strFilename, size_t num)
{
	std::ofstream fout(strFilename.c_str(), std::ios::out);

	// setup the stream so that things are formatted properly
	fout.setf(std::ios::scientific | std::ios::left);
	fout.precision(8);

	if( fout.bad() ) {
		std::cerr << std::endl << "Could not open " << strFilename << " for output.";
		return false;
	}

	fout << "Dangerplot_version\t" << "2.0" << std::endl;
	fout << "Number_of_points\t" << m_cvmFID[num].size() << std::endl;
	fout << "Sampling_frequency\t" << GetSamplingFrequency() << std::endl;
	fout << "Transmitter_frequency\t" << GetTransmitterFrequency() << std::endl;
	fout << "Phi0\t" << m_phi0[num].first << std::endl;
	fout << "Phi1\t" << m_phi1[num].first << std::endl;
	fout << "PPM_reference\t" << m_ref[num].first << std::endl;
	fout << "Echo_time\t" << GetEchoTime() << std::endl;
	fout << "Real_FID\t" << "Imag_FID\t" << std::endl;

	for(int n = 0; n < m_cvmFID[num].size(); n++)
		fout << real(m_cvmFID[num][n+1]) << "\t" << imag(m_cvmFID[num][n+1]) << std::endl;

	return true;
}

// assumes you want the first FID for multi-voxel fitting
bool tarquin::CFID::SaveToFileV3(std::string strFilename)
{
	std::ofstream fout(strFilename.c_str(), std::ios::out);

	// setup the stream so that things are formatted properly
	fout.setf(std::ios::scientific | std::ios::left);
	fout.precision(8);

	if( fout.bad() ) {
		std::cerr << std::endl << "Could not open " << strFilename << " for output.";
		return false;
	}

	fout << "Dangerplot_version\t" << "3.0" << std::endl;
	fout << "Number_of_points\t" << m_cvmFID[0].size() << std::endl;
	fout << "Sampling_frequency\t" << GetSamplingFrequency() << std::endl;
	fout << "Transmitter_frequency\t" << GetTransmitterFrequency() << std::endl;
	fout << "Phi0\t" << m_phi0[0].first << std::endl;
	fout << "Phi1\t" << m_phi1[0].first << std::endl;
	fout << "PPM_reference\t" << m_ref[0].first << std::endl;
	fout << "Echo_time\t" << GetEchoTime() << std::endl;
	fout << "Rows\t" << m_rows << std::endl;
	fout << "Cols\t" << m_cols << std::endl;
	fout << "Slices\t" << m_slices << std::endl;
    if ( IsKnownVoxelDim() )
    {
        const std::vector<double>& voxel_dim = GetVoxelDim();
        fout << "PixelSpacing\t" << voxel_dim[0] << "\\" << voxel_dim[1] << std::endl;
        fout << "SliceThickness\t" << voxel_dim[2] << std::endl;
    }
    else
    {
        fout << "PixelSpacing\tUnknown" << std::endl;
        fout << "SliceThickness\tUnknown" << std::endl;
    }
    
    if ( IsKnownRowDirn() && IsKnownColDirn() )
    {
        fout << "ImageOrientationPatient\t";
        std::string row_ori_str;
        rvec2str(GetRowDirn(),row_ori_str);
        std::string col_ori_str;
        rvec2str(GetColDirn(),col_ori_str);
        fout << row_ori_str + "\\" + col_ori_str << std::endl;
    }
    else
    {
        fout << "ImageOrientationPatient\tUnknown" << std::endl;
    }

    if ( IsKnownPos() )
    {
        fout << "ImagePositionPatient\t";
        std::string pos_str;
        rvec2str(GetPos(), pos_str);
        fout << pos_str << std::endl;
    }
    else
    {
        fout << "ImagePositionPatient\tUnknown" << std::endl;
    }

	fout << "Real_FID\t" << "Imag_FID\t" << std::endl;

	for(int m = 0; m < m_cvmFID.size(); m++)
        for(int n = 0; n < m_cvmFID[m].size(); n++)
            fout << real(m_cvmFID[m][n+1]) << "\t" << imag(m_cvmFID[m][n+1]) << std::endl;

	return true;
}

// assumes you want the first FID for multi-voxel fitting
bool tarquin::CFID::SaveToFileLCM(std::string strFilename)
{
	std::ofstream fout(strFilename.c_str(), std::ios::out);

	// setup the stream so that things are formatted properly
	fout.setf( std::ios::scientific | std::ios::right | std::ios::uppercase );

	if( fout.bad() ) {
		std::cerr << std::endl << "Could not open " << strFilename << " for output.";
		return false;
	}

	fout << " $NMID" << std::endl;
	fout << " ID='Simulated Data', FMTDAT='(2E15.6)'" << std::endl;
	fout << " VOLUME=1" << std::endl;
	fout << " TRAMP=1" << std::endl;
	fout << " $END" << std::endl;

	//fout.fill('*');

	fout.precision(6);
	for(int n = 0; n < m_cvmFID[0].size(); n++) {
		fout.width(15);
		fout << real(m_cvmFID[0][n+1]);
		fout.width(15);
		fout << imag(m_cvmFID[0][n+1]) << std::endl;
	}

	return true;
}

// the point of this is that so numerical tolerances always apply to numbers in the same range
void tarquin::CFID::Normalise()
{
	assert( m_cvmFID[0].size() > 0 ); //TODO, function could be defunct

	// the biggest point in the time domain will have magnitude of 1
	m_norm_val = m_cvmFID[0].norminf(); //TODO

	// now divide every element by this normalising value
	m_cvmFID[0] /= m_norm_val; //TODO
}

// call the appropriate loading function depending on the format
void tarquin::CFID::Load(std::string strFilename, Options& options, Workspace& workspace, CBoswell& log)
{
    std::string str_out = "Loading data file : " + strFilename;
    log.LogMessage(LOG_INFO, str_out.c_str());

    // make sure fid data is empty
    ClearVectors();

	if( tarquin::NOTSET == options.GetFormat() ) 
	{
		throw Exception("the format was not specified");
	}
	else if( tarquin::DANGER == options.GetFormat() ) 
	{
		CFIDReaderDPT reader(*this, log);
		reader.Load(strFilename, options, log);
	}
	else if( tarquin::SIEMENS == options.GetFormat() ) 
	{
		CFIDReaderSiemens reader(*this, log);
		reader.Load(strFilename, options, log);
	}
    else if( tarquin::DCM == options.GetFormat() ) 
	{
		CFIDReaderDCM reader(*this, log);
		reader.Load(strFilename, options, log);
	}
	else if( tarquin::RDA == options.GetFormat() ) 
	{
		CFIDReaderRDA reader(*this, log);
		reader.Load(strFilename, options, log);
	}
	else if( tarquin::PHILIPS == options.GetFormat() ) 
	{
		CFIDReaderPhilips reader(*this, log);
		reader.Load(strFilename, options, log);
        int m_rows_t = m_rows;
        int m_cols_t = m_cols;
        m_rows = m_cols_t;
        m_cols = m_rows_t;
        swap_row_col();
	}
	else if( tarquin::PHILIPS_DCM == options.GetFormat() ) 
	{
		CFIDReaderPhilipsDCM reader(*this, log);
		reader.Load(strFilename, options, log);
	}
	else if( tarquin::GE == options.GetFormat() ) 
	{
		CFIDReaderGE reader(*this, log);
		reader.SetOptions(options.GetGEOptions());
		reader.Load(strFilename, options, log);
	}
	else if( tarquin::SHF == options.GetFormat() ) 
	{
		CFIDReaderGE reader(*this, log);
		reader.LoadSHF(strFilename, options, true, log);
	}
	else if( tarquin::LCM == options.GetFormat() ) 
	{
		CFIDReaderLCM reader(*this, log);
		reader.Load(strFilename, options, log);
	}
	else if( tarquin::VARIAN == options.GetFormat() ) 
	{
		CFIDReaderVarian reader(*this, log);
		reader.Load(strFilename, options, log);
	}
	else if( tarquin::BRUKER == options.GetFormat() ) 
	{
		CFIDReaderBruker reader(*this, log);
		reader.Load(strFilename, options, log);
	}
    else if( tarquin::JMRUI_TXT == options.GetFormat() ) 
	{
		CFIDReaderJMRUITXT reader(*this, log);
		reader.Load(strFilename, options, log);
	}

    // swap the rows and columns if requested
    if ( options.GetSwapRowCol() )
    {
        swap_row_col();
        int f_rows = options.GetFitRows();
        int f_cols = options.GetFitCols();
        options.SetFitRows(f_cols);
        options.SetFitCols(f_rows);
    }

    // filter k-space if requested
    if ( options.GetFilterKspace())
        filter_kspace(options, log);

    // zero-fill k-space if requested
    if ( options.GetZfillKspace() != 1 )
        zfill_kspace(options.GetZfillKspace(), options, log);
    
    //trans_kspace(options, log);

	// set the point range to sensible values if not set to something else 
	if( 0 == options.GetRangeStart() && options.GetPulSeq() == MEGA_PRESS )
    {
        options.SetRangeStart(10);
    }
    else if ( 0 == options.GetRangeStart() && !options.GetAdaptSp() )
    {
        double range_st = options.GetRangeStartTime();
        options.SetRangeStart(round(GetSamplingFrequency()*range_st));
        if ( round(GetSamplingFrequency()*range_st) < options.GetMinRange() )
            options.SetRangeStart(options.GetMinRange());
    }

    // this should be calculated automatically by looking at the fid
	if( 0 == options.GetRangeEnd() )
    {
        if ( GetNumberOfPoints() == 512 )
		    options.SetRangeEnd(500);
        else
            options.SetRangeEnd(GetNumberOfPoints()/2);
    }

    log.LogMessage(LOG_INFO, "nStart = %i",options.GetRangeStart());
    log.LogMessage(LOG_INFO, "nEnd   = %i",options.GetRangeEnd());

	// adjust FID to have unit length in l-infinity norm of FD
	// WTF is this doing here?????????? MW 17Jun09
	//Normalise();
	
	// set the default voxel(s) to be analysed
	coord_vec fit_list;
    fit_list.clear();

    /*
    std::cout << "Voxels:" << m_cols * m_rows * m_slices << std::endl;
    std::cout << "Cols:  " << m_cols << std::endl;
    std::cout << "Rows:  " << m_rows << std::endl;
    std::cout << "Slices:" << m_slices << std::endl;
    */

    /*if ( NONE != options.GetDynAv() )
    {
        // looks like we're doing some sort of dynamic averaging
    }*/


    if ( !m_wref )
    {
        // SVS	
        if ( m_cols * m_rows * m_slices == 1 )
        {
            coord fit_spec(1, 1, 1); 
            fit_list.push_back(fit_spec);
        }
        else
        {
            if ( options.GetSVSOnly() && !m_dyn )
            {
                std::cout << std::endl << "Cols   " << m_cols << std::endl;
                std::cout << "Rows   " << m_rows << std::endl;
                std::cout << "Slices " << m_slices << std::endl;
                std::cout << "Dyn    " << m_dyn << std::endl;
                log.LogMessage(LOG_ERROR, "Error, more than one voxel in fid and svs_only set to true.");
                throw Exception("Error, more than one voxel in fid and svs_only set to true.");
            }

            // added to just make all voxels selected for analysis
            m_voi_dim.second = false;

            if ( IsKnownVoxelDim() && IsKnownVoiDim() )
            {
                if ( options.GetFitRows() == -1 )
                {
                    int fit_rows = int(m_voi_dim.first[0] / m_voxel_dim.first[0]);
                    if (fit_rows % 2 == 0)
                        options.SetFitRows(fit_rows);
                    else
                        options.SetFitRows(fit_rows-1);
                }

                if ( options.GetFitCols() == -1 )
                {
                    int fit_cols = int(m_voi_dim.first[1] / m_voxel_dim.first[1]);
                    if (fit_cols % 2 == 0)
                        options.SetFitCols(fit_cols);
                    else
                        options.SetFitCols(fit_cols-1);
                }
            }
            else
            {
                if ( options.GetFitRows() == -1 )
                    options.SetFitRows(m_rows);
                if ( options.GetFitCols() == -1 )
                    options.SetFitCols(m_cols);
            }

            if ( options.GetFitSlices() == -1 )
                options.SetFitSlices(m_slices);

            // CSI full
            const int row_start   = static_cast<int>(m_rows/2.0 - options.GetFitRows()/2.0 + 1.0);
            const int row_end     = static_cast<int>(m_rows/2.0 + options.GetFitRows()/2.0);
            const int col_start   = static_cast<int>(m_cols/2.0 - options.GetFitCols()/2.0 + 1.0);
            const int col_end     = static_cast<int>(m_cols/2.0 + options.GetFitCols()/2.0);
            const int slice_start = static_cast<int>(m_slices/2.0 - options.GetFitSlices()/2.0 + 1.0);
            const int slice_end   = static_cast<int>(m_slices/2.0 + options.GetFitSlices()/2.0);

            for ( int slice = slice_start; slice < slice_end + 1; slice++ )
            {
                for ( int col = col_start; col < col_end + 1; col++ )
                {
                    for ( int row = row_start; row < row_end + 1; row++ )
                    {
                        coord fit_spec(row, col, slice); 
                        fit_list.push_back(fit_spec);
                    }
                }
            }
        }
        options.SetFitList(fit_list);
    }

	// set default values for some parameters
    // if ref has been specified then override
    // the current value (should only be needed for dpt files)
    // since no other data formats change the ref value when loading

    std::pair<treal, bool> def_ref;
    if ( options.GetRefSpec() ) // has ref been specified yet?
    {
        // set to options value
        def_ref.first = options.GetRef();
        def_ref.second = true;
    }
    else
    {
        // keep as-is
        def_ref = m_ref[0];
    }

    //std::cout << std::endl << "m_ref [0]   : " << m_ref[0].first << std::endl;
    //std::cout << std::endl << "options ref : " << options.GetRef() << std::endl;
    //std::cout << "m_ref size : " << m_ref.size() << std::endl;

    pair_vec ref_vec;
	for ( int n = 0; n < m_rows * m_cols * m_slices; n++ )
		ref_vec.push_back(def_ref);
    m_ref = ref_vec;

    //std::cout << "m_ref size : " << m_ref.size() << std::endl;
    //std::cout << "voxels     : " << m_rows * m_cols * m_slices << std::endl;
    //assert( m_ref.size() == m_rows * m_cols * m_slices );

    std::pair<treal, bool> def_phi0 = m_phi0[0];
    pair_vec phi0_vec;
	for ( int n = 0; n < m_rows * m_cols * m_slices; n++ )
        phi0_vec.push_back(def_phi0);

    m_phi0 = phi0_vec;
    assert( m_phi0.size() == m_rows * m_cols * m_slices );

    std::pair<treal, bool> def_phi1 = m_phi1[0];
    pair_vec phi1_vec;
	for ( int n = 0; n < m_rows * m_cols * m_slices; n++ )
        phi1_vec.push_back(def_phi1);

    m_phi1 = phi1_vec;
    assert( m_phi1.size() == m_rows * m_cols * m_slices );

    // apply a shift
    // ShiftGrid(0, 0, 1, log);

    // perform dynamic frequency correction of WS data if requested
    if ( options.GetDynFreqCorr() && !m_wref )
    {
        size_t n = 0;
        for ( int slice = 1; slice < m_slices + 1; slice++ )
        {
            for ( int col = 1; col < m_cols + 1; col++ )
            {
                for ( int row = 1; row < m_rows + 1; row++ )
                {
                    coord scan(row, col, slice); 
                    if ( options.GetPDFC() )
                    {
                        if ( n % 2 == 0 ) // if even
                        {
                            AutoReferenceCorr(scan, options, *this, true, false, true, log);
                        }
                        else
                        {
                            int m = vox2ind(scan);
                            m_ref[m].first = m_ref[m-1].first;
                            m_ref[m].second = true;

                        }
                    }
                    else
                    {
                        AutoReferenceCorr(scan, options, *this, true, false, true, log);
                    }
                    n++;
                }
            }
        }

        // clear just in case 
        workspace.ClearDynShift();
        for ( size_t m = 0; m < m_ref.size(); m++ )
        {
            treal shift_hz = ( options.GetRef() - m_ref[m].first ) * GetTransmitterFrequency() / 1e6;
            workspace.AppendDynShift(shift_hz);
        }

        if( options.GetDynShiftFilename() != "" )
        {
            log.LogMessage(LOG_INFO, "Writing dynamic shifts to file.");
            // write to txt file
            std::string fname = options.GetDynShiftFilename();
            std::ofstream shiftfile(fname.c_str());
            if ( shiftfile.is_open() )
            {
                for ( size_t m = 0; m < m_ref.size(); m++ )
                {
                    treal shift_hz = ( options.GetRef() - m_ref[m].first ) * GetTransmitterFrequency() / 1e6;
                    shiftfile << m+1 << "," << shift_hz << std::endl;
                }
                shiftfile.close();
            }
        }

        // hard shift data
        for ( size_t m = 0; m < m_ref.size(); m++ )
            ShiftRef(options.GetRef(), m);

    }
    
    if ( options.GetDynAv() == DEFAULT && !m_wref )
    {
        if ( m_dyn )
            options.SetDynAv(ALL);
        else
            options.SetDynAv(NONE);
    }

    if ( options.GetDynAvW() == DEFAULT && m_wref )
    {
        if ( m_dyn )
            options.SetDynAvW(ALL);
        else
            options.SetDynAvW(NONE);
    }

    if ( options.GetInvertEvenPairs() )
        InvertEvenPairs(options, log);

    if ( options.GetDynAv() != NONE )
    {
        int WUS_fids = workspace.GetFIDRaw().GetVoxelCount();
        if ( WUS_fids != m_cvmFID.size() )
            AverageData(options, log, WUS_fids);
        else
            AverageData(options, log);
    }

    // check if we need to average WUS data due to missmatch between WUS and WS scans
    if ( m_wref )
    {
        int WUS_fids = workspace.GetFIDRaw().GetVoxelCount();
        //std::cout << std::endl << WUS_fids << std::endl;
        //std::cout << m_cvmFID.size() << std::endl;
        if ( WUS_fids != m_cvmFID.size() )
            AverageData(options, log, WUS_fids);
    }

    if ( options.GetFullEcho() )
        trim_echo();

}

// call the appropriate loading function depending on the format
void tarquin::CFID::LoadW(std::string strFilename, Options& options, CBoswell& log)
{
    // flag as a water ref file
    m_wref = true;

    // make sure fid data is empty
    ClearVectors();

	if( tarquin::NOTSET == options.GetFormat() ) 
	{
		throw Exception("the format was not specified");
	}
	else if( tarquin::GE == options.GetFormat() ) 
	{
		CFIDReaderGE reader(*this, log);
		reader.SetOptions(options.GetGEOptions());
		reader.LoadW(strFilename, options, log);
	}
	else if( tarquin::SHF == options.GetFormat() ) 
	{
		CFIDReaderGE reader(*this, log);
		reader.LoadSHF(strFilename, options, false, log);
	}
    else if( tarquin::DCM == options.GetFormat() ) 
	{
		CFIDReaderDCM reader(*this, log);
		reader.LoadW(strFilename, options);
	}
    
    // swap the rows and columns if requested
    if ( options.GetSwapRowCol() )
    {
        swap_row_col();
        int f_rows = options.GetFitRows();
        int f_cols = options.GetFitCols();
        options.SetFitRows(f_cols);
        options.SetFitCols(f_rows);
    }

    // filter k-space if requested
    if ( options.GetFilterKspace())
        filter_kspace(options, log);

    // zero-fill k-space if requested
    if ( options.GetZfillKspace() != 1 )
        zfill_kspace(options.GetZfillKspace(), options, log);

    //trans_kspace(options, log);

    // set default values for some parameters
    std::pair<treal, bool> def_ref;
    if ( options.GetRefSpec() )
    {
        // set to options value
        def_ref.first = options.GetRef();
        def_ref.second = true;
    }
    else
    {
        // keep as-is
        def_ref = m_ref[0];
    }
    pair_vec ref_vec;
	for ( int n = 0; n < m_rows * m_cols * m_slices; n++ )
		ref_vec.push_back(def_ref);

    m_ref = ref_vec;

    std::pair<treal, bool> def_phi0 = m_phi0[0];
    pair_vec phi0_vec;
	for ( int n = 0; n < m_rows * m_cols * m_slices; n++ )
        phi0_vec.push_back(def_phi0);

    m_phi0 = phi0_vec;

    std::pair<treal, bool> def_phi1 = m_phi1[0];
    pair_vec phi1_vec;
	for ( int n = 0; n < m_rows * m_cols * m_slices; n++ )
        phi1_vec.push_back(def_phi1);

    m_phi1 = phi1_vec;

    if ( options.GetDynAvW() == DEFAULT )
    {
        if ( m_dyn )
        {
            options.SetDynAvW(ALL);
        }
        else
        {
            options.SetDynAvW(NONE);
        }
    }

    if ( options.GetInvertEvenPairs() )
        InvertEvenPairs(options, log);

    // shouldn't ever be default at this point because WUS data is always loaded after WS data
    if ( options.GetDynAvW() != NONE )
        AverageData(options, log);

    //std::cout << "Parameter Water ref fids :" << m_cols * m_rows * m_slices << std::endl;
    //std::cout << "Actual water ref fids    :" << m_cvmFID.size() << std::endl;

    // check if we need to average WUS data due to missmatch between WUS and WS scans
    if ( m_wref && (m_cols * m_rows * m_slices != m_cvmFID.size()) )
    {
        //std::cout << "I'm here" << std::endl;
        AverageData(options, log);
    }
}

void tarquin::CFID::InvertEvenPairs(Options& options, CBoswell& log)
{
    for ( size_t n = 0; n < m_cvmFID.size(); n++ )
    {
        if ( int(std::floor(n/2.0)) % 2 != 0 ) // n odd?
            m_cvmFID[n] = m_cvmFID[n]*std::exp( tcomplex(0, M_PI) );
    }
}

void tarquin::CFID::AverageData(Options& options, CBoswell& log, int missmatch)
{
    if ( !m_wref )
        log.LogMessage(LOG_INFO, "Averaging metabolite FIDs");
    else
        log.LogMessage(LOG_INFO, "Averaging water ref FIDs");

    int pts = GetNumberOfPoints();

    std::vector<int> av_list;
    
    // read in the av_file if specified
    if ( options.GetAvListFile() != "" )
    {
        std::ifstream infile(options.GetAvListFile().c_str());  // need some eror checking here
        int num;
        while (infile >> num)
            av_list.push_back(num-1);
    }
    else
    {
        for ( size_t n = 0; n < m_cvmFID.size(); n++ )
            av_list.push_back(n);
    }
    
    int fids = av_list.size();
    int data_fids = m_cvmFID.size();

    //std::cout << "Averaging data" << std::endl;

    //std::cout << fids << std::endl;

    /*if ( fids % 2 && fids > 1 ) // odd fids are bad
    {
        std::cout << std::endl << "An odd number of FIDs is bad." << std::endl;
        assert(0);
    }*/

    cvm::cvector new_fid(pts);

    if ( !m_wref ) 
    {
        log.LogMessage(LOG_INFO, "Averaging across %i of %i FIDs", fids, data_fids);
        for ( size_t p = 0; p < av_list.size(); p++ )
        {
            int n = av_list[p];
            if ( options.GetDynAv() == ALL )
                new_fid = new_fid + m_cvmFID[n] / fids;
            else if ( options.GetDynAv() == ODD && ( (n + 1) % 2 != 0 ) ) // n odd?
                new_fid = new_fid + m_cvmFID[n] / (fids / 2);
            else if ( options.GetDynAv() == EVEN && ( (n + 1) % 2 == 0 ) ) // n even?
                new_fid = new_fid + m_cvmFID[n] / (fids / 2);
            else if ( options.GetDynAv() == SUBTRACT && ( (n + 1) % 2 != 0 ) ) // n odd?
                new_fid = new_fid + m_cvmFID[n] / fids;
            else if ( options.GetDynAv() == SUBTRACT && ( (n + 1) % 2 == 0 ) ) // n even?
                new_fid = new_fid - m_cvmFID[n] / fids;
            else if ( options.GetDynAv() == DEFAULT )
            {
                std::cout << "ERROR, averaging should not be default here." << std::endl;
                new_fid = new_fid + m_cvmFID[n] / fids;
            }
        }
    }
    else
    {

    if ( missmatch > 0 || data_fids != m_cols * m_rows * m_slices) // if we got here it's because of a missmatch between WS and W dyanmics
    {
        log.LogMessage(LOG_INFO, "Number of W fids does not match WS fids.");
        log.LogMessage(LOG_INFO, "Averaging accross %i water reference fids.",data_fids);
        for ( size_t n = 0; n < data_fids; n++ )
        {
            if ( options.GetDynAvW() == ALL )
                new_fid = new_fid + m_cvmFID[n] / data_fids;
            else if ( options.GetDynAvW() == ODD && ( (n + 1) % 2 != 0 ) ) // n odd?
                new_fid = new_fid + m_cvmFID[n] / (data_fids / 2);
            else if ( options.GetDynAvW() == EVEN && ( (n + 1) % 2 == 0 ) ) // n even?
                new_fid = new_fid + m_cvmFID[n] / (data_fids / 2);
            else if ( options.GetDynAvW() == SUBTRACT && ( (n + 1) % 2 != 0 ) ) // n odd?
                new_fid = new_fid + m_cvmFID[n] / data_fids;
            else if ( options.GetDynAvW() == SUBTRACT && ( (n + 1) % 2 == 0 ) ) // n even?
                new_fid = new_fid - m_cvmFID[n] / data_fids;
            else if ( options.GetDynAvW() == NONE ) // we shouldn't be here, error! 
                new_fid = new_fid + m_cvmFID[n] / data_fids;
            else if ( options.GetDynAvW() == DEFAULT )
            {
                std::cout << "ERROR, averaging should not be default here." << std::endl;
                new_fid = new_fid + m_cvmFID[n] / data_fids;
            }
        }
    }
    else
    {
        log.LogMessage(LOG_INFO, "Number of W fids and WS fids is the same.", fids, data_fids);
        log.LogMessage(LOG_INFO, "Averaging across a subset (%i of %i) water reference FIDs.", fids, data_fids);
        for ( size_t p = 0; p < av_list.size(); p++ )
        {
            int n = av_list[p];
            if ( options.GetDynAvW() == ALL )
                new_fid = new_fid + m_cvmFID[n] / fids;
            else if ( options.GetDynAvW() == ODD && ( (n + 1) % 2 != 0 ) ) // n odd?
                new_fid = new_fid + m_cvmFID[n] / (fids / 2);
            else if ( options.GetDynAvW() == EVEN && ( (n + 1) % 2 == 0 ) ) // n even?
                new_fid = new_fid + m_cvmFID[n] / (fids / 2);
            else if ( options.GetDynAvW() == SUBTRACT && ( (n + 1) % 2 != 0 ) ) // n odd?
                new_fid = new_fid + m_cvmFID[n] / fids;
            else if ( options.GetDynAvW() == SUBTRACT && ( (n + 1) % 2 == 0 ) ) // n even?
                new_fid = new_fid - m_cvmFID[n] / fids;
            else if ( options.GetDynAvW() == NONE ) // we shouldn't be here, error! 
                new_fid = new_fid + m_cvmFID[n] / fids;
            else if ( options.GetDynAvW() == DEFAULT )
            {
                std::cout << "ERROR, averaging should not be default here." << std::endl;
                new_fid = new_fid + m_cvmFID[n] / fids;
            }
        }
    }


        /*
        for ( size_t n = 0; n < fids; n++ )
            new_fid = new_fid + m_cvmFID[n] / fids;

        if ( options.GetDynAv() == DEFAULT )
            std::cout << "ERROR, averaging should not be default here." << std::endl;
            */
    }

    m_cvmFID.clear();

    /*std::cout << m_cols << std::endl;
    std::cout << m_rows << std::endl;
    std::cout << m_slices << std::endl;*/
    
    // do we need to duplicate the fid?
    if ( (data_fids != m_cols * m_rows * m_slices) )
    {
        //log.LogMessage(LOG_INFO,"Duplicating water reference FIDs to match number of metabolite FIDs (%i).",m_cols * m_rows * m_slices);
        //log.LogMessage(LOG_INFO,"Duplicating water reference FIDs to match number of metabolite FIDs.",m_cols * m_rows * m_slices);
        if ( m_wref ) // are we water data?
        {
            for ( size_t n = 0; n < m_cols * m_rows * m_slices; n++ )
                m_cvmFID.push_back(new_fid);
        }
        else
        {
            std::cout << std::endl << "Mismatch in fid numbers." << std::endl;
            assert(0);
        }
    }
    else if ( missmatch > 0 )
    {
        if ( m_wref ) // are we water data?
        {
            for ( size_t n = 0; n < missmatch; n++ )
                m_cvmFID.push_back(new_fid);

            m_cols = missmatch;
        }
        else
        {
            std::cout << std::endl << "Mismatch in fid numbers." << std::endl;
            assert(0);
        }
    }
    else // just the one fid out after averaging then
    {
        m_cvmFID.push_back(new_fid);
        m_cols = 1;
        m_rows = 1;
        m_slices = 1;
        coord_vec fit_list;
        fit_list.clear();
        coord fit_spec(1, 1, 1); 
        fit_list.push_back(fit_spec);
        options.SetFitList(fit_list);
    }

    std::pair<treal, bool> def_ref = m_ref[0];
    pair_vec ref_vec;
	for ( int n = 0; n < m_rows * m_cols * m_slices; n++ )
		ref_vec.push_back(def_ref);
    m_ref = ref_vec;

    std::pair<treal, bool> def_phi0 = m_phi0[0];
    pair_vec phi0_vec;
	for ( int n = 0; n < m_rows * m_cols * m_slices; n++ )
        phi0_vec.push_back(def_phi0);
    m_phi0 = phi0_vec;

    std::pair<treal, bool> def_phi1 = m_phi1[0];
    pair_vec phi1_vec;
	for ( int n = 0; n < m_rows * m_cols * m_slices; n++ )
        phi1_vec.push_back(def_phi1);
    m_phi1 = phi1_vec;
}

void tarquin::CFID::FreqCorrData(const Options& options)
{
    int pts = GetNumberOfPoints();
    int fids = m_cvmFID.size();
    cvm::cvector new_fid(pts);
    for ( size_t n = 0; n < fids; n++ )
    {
        new_fid = new_fid + m_cvmFID[n];
    }
}

cvm::rvector tarquin::CFID::GetRawMap()
{
    int fids = m_cvmFID.size();
	cvm::rvector map(fids);
    for ( size_t n = 0; n < fids; n++ )
        map(n+1) = m_cvmFID[n].norm();

    return map;
}

