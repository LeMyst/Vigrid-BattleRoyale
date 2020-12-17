using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Amazon;
using Amazon.S3;
using Amazon.S3.Model;

namespace WinServInstaller
{
    class ObjectStore
    {
        private AmazonS3Client client;
        private string Bucket;
        public ObjectStore(string accessKey, string secretKey, string serviceUrl = "https://ewr1.vultrobjects.com") //default to VULTR for object store
        {
            AmazonS3Config config = new AmazonS3Config();
            config.ServiceURL = serviceUrl;

            client = new AmazonS3Client(accessKey, secretKey, config);
        }

        public void TestConnection()
        {
            
            ListBucketsResponse response = client.ListBuckets();

            foreach (var bucket in response.Buckets)
            {
                string name = bucket.BucketName;
            }
        }

        public void SetBucket(string bucket_name)
        {
            this.Bucket = bucket_name;
        }

        public string[] GetAllkeys(string path)
        {
            List<string> keys = new List<string>();
            ListObjectsV2Response resp = client.ListObjectsV2(new ListObjectsV2Request()
            {
                BucketName = this.Bucket,
                Prefix = path
            });
            foreach (var obj in resp.S3Objects)
            {
                if(obj.Size > 0)
                {
                    keys.Add(obj.Key);
                }
            }
            return keys.ToArray();
        }
        public string GetLatestKey(string path)
        {
            ListObjectsV2Response resp = client.ListObjectsV2(new ListObjectsV2Request()
            {
                BucketName = this.Bucket,
                Prefix = path
            });

            S3Object latest = null;
            foreach(var obj in resp.S3Objects)
            {
                if(obj.Size > 0)
                {
                    if (latest == null)
                    {
                        latest = obj;
                        continue;
                    }
                    if(obj.LastModified > latest.LastModified)
                    {
                        latest = obj;
                    }
                }
                    
            }
            Console.WriteLine("Found latest key: " + latest.Key);
            return latest.Key;
        }

        public void WriteToFile(string file_path, string object_key)
        {
            GetObjectResponse resp = client.GetObject(this.Bucket, object_key);
            resp.WriteResponseStreamToFile(file_path);
        }
    }
}
