
#include "Reconstruct.hpp"

#include <boost/thread/thread.hpp>
#include <pcl/console/parse.h>
#include <pcl/io/pcd_io.h>
#include <pcl/io/ply_io.h>

#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/surface/mls.h>


using namespace pcl;
using namespace std;
using namespace reconstruction;

Reconstruct::Reconstruct():
    sourceCloud(new PointCloud<PointXYZRGB> ())
{}


void Reconstruct::showHelp(char *programName){
    cout << "-- Reconstruct: usage: " << programName << " cloud_filename.[pcd|ply]" << endl;
    cout << "-- Reconstruct: exit program. " << endl;
    exit(EXIT_FAILURE);
}


void Reconstruct::openPCL(int argc, char** argv){
    // Fetch point cloud filename in arguments | Works with PCD and PLY files
    vector<int> filenames;
    bool file_is_pcd = false;
	
    filenames = console::parse_file_extension_argument(argc,argv,".ply");

    if(filenames.size() != 1){
        filenames = console::parse_file_extension_argument(argc,argv,".pcd");

        if(filenames.size() != 1)
        {
            showHelp(argv[0]);
        } else{
            file_is_pcd = true;
        }
    }
	
    // Load file | Works with PCD and PLY files
    if(file_is_pcd){
        if(io::loadPCDFile(argv[filenames[0]],*sourceCloud) < 0){
            cout << "-- Reconstruct: error loading point cloud " << argv[filenames[0]] << endl << endl;
            showHelp(argv[0]);
        } 
    }else {
        if(io::loadPLYFile(argv[filenames[0]],*sourceCloud) < 0){
            cout << "-- Reconstruct: error loading point cloud " << argv[filenames[0]] << endl << endl;
            showHelp (argv[0]);
        }
    }

    cout << "-- Reconstruct: point cloud " << argv[filenames[0]] << " successfully loaded" << endl;
}


void Reconstruct::statisticalFilter(int meanK,float stdDevMulThresh,
    PointCloud<PointXYZRGB>::Ptr filteredCloud)
{
	// Statistical filter object
	StatisticalOutlierRemoval<PointXYZRGB> statisticalFilter;
	statisticalFilter.setInputCloud(sourceCloud);

	// Every point must have 10 neighbors within 15cm, or it will be removed
    cout << "-- Reconstruct: loading filter meanK value = " << meanK << endl;
    statisticalFilter.setMeanK( meanK );

    cout << "-- Reconstruct: loading filter stdDevMulThresh value = " << stdDevMulThresh << endl;
    statisticalFilter.setStddevMulThresh( stdDevMulThresh );
    statisticalFilter.filter(*filteredCloud);
}


void Reconstruct::polynomialInterpolation(float kdtreeRadius,
            PointCloud<PointXYZRGB>::Ptr filteredCloud,
            PointCloud<PointXYZRGB>::Ptr smoothedCloud){
    // http://pointclouds.org/documentation/tutorials/resampling.php#moving-least-squares
    // attempts to recreate the missing parts of the surface by higher order polynomial interpolations 
    // Init object
    MovingLeastSquares<PointXYZRGB, PointXYZRGB> mls;
    mls.setComputeNormals(true);

    // Set parameters
    mls.setInputCloud(filteredCloud);
    mls.setPolynomialFit(true);
    mls.setPolynomialOrder(2);


    // SAMPLE_LOCAL_PLANE   RANDOM_UNIFORM_DENSITY VOXEL_GRID_DILATION
    mls.setUpsamplingMethod(MovingLeastSquares<PointXYZRGB, PointXYZRGB>::SAMPLE_LOCAL_PLANE);
    mls.setUpsamplingMethod(MovingLeastSquares<PointXYZRGB, PointXYZRGB>::RANDOM_UNIFORM_DENSITY);
    mls.setUpsamplingRadius(0.10);
    mls.setUpsamplingStepSize(0.03);
    // add multiple threads ??

    search::KdTree<PointXYZRGB>::Ptr tree (new search::KdTree<PointXYZRGB>);
    mls.setSearchMethod(tree);

    cout << "-- Reconstruct: loading kdtree radius value = " << kdtreeRadius << endl;
    mls.setSearchRadius(kdtreeRadius);

    mls.process(*smoothedCloud);
}


void Reconstruct::visualizeCloud(PointCloud<PointXYZRGB>::Ptr outputCloud, string windowTitle){
    // Visualize the output point cloud
    boost::shared_ptr<visualization::PCLVisualizer> viewer(new visualization::PCLVisualizer( windowTitle ));
    visualization::PointCloudColorHandlerRGBField<PointXYZRGB> rgb(outputCloud);
    viewer->addPointCloud(outputCloud,rgb, windowTitle);

    while (!viewer->wasStopped())
    {
        viewer->spinOnce(100);
        boost::this_thread::sleep(boost::posix_time::microseconds(100000));
    }
}



