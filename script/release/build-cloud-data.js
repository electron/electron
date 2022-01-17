// RETURNED FROM QUERRYING ALL CLOUDS
// Res:  [
//     {
//       buildCloudId: 861,
//       accountId: 0,
//       name: 'electron-16-core',
//       groupName: '',
//       workersCapacity: 40,
//       cloudType: 'Azure',
//       disabled: false,
//       status: 'Active',
//       busyWorkers: 0,
//       buildJobsCount: 0,
//       load: 0,
//       lastFailed: '2022-01-04T14:37:31.0431778+00:00',
//       failureBackoffTimeSeconds: 30,
//       created: '0001-01-01T00:00:00+00:00'
//     },
//     {
//       buildCloudId: 1424,
//       accountId: 0,
//       name: 'electron-16-core2',
//       groupName: '',
//       workersCapacity: 20,
//       cloudType: 'Azure',
//       disabled: false,
//       status: 'Active',
//       busyWorkers: 0,
//       buildJobsCount: 0,
//       load: 0,
//       failureBackoffTimeSeconds: 30,
//       created: '0001-01-01T00:00:00+00:00'
//     },
//     {
//       buildCloudId: 682,
//       accountId: 0,
//       name: 'libcc-20',
//       groupName: '',
//       workersCapacity: 40,
//       cloudType: 'Azure',
//       disabled: false,
//       status: 'Active',
//       busyWorkers: 0,
//       buildJobsCount: 0,
//       load: 0,
//       lastFailed: '2022-01-04T15:00:02.584313+00:00',
//       failureBackoffTimeSeconds: 30,
//       created: '0001-01-01T00:00:00+00:00'
//     }
//   ]

// RETURNED THE IMAGE
// Res:  {
//     buildCloudId: 682,
//     accountId: 59457,
//     name: 'libcc-20',
//     workersCapacity: 40,
//     cloudType: 'Azure',
//     disabled: false,
//     settings: {
//       availabilityStrategy: {},
//       failureStrategy: {
//         jobStartTimeoutSeconds: 1200,
//         provisioningAttempts: 5,
//         failureAttempts: 10,
//         failureBackoffTimeSeconds: 30,
//         alternateCloudOrGroupName: 'electron-16-core'
//       },
//       cloudSettings: {
//         azureAccount: [Object],
//         vmConfiguration: [Object],
//         networking: [Object],
//         images: [Array],
//         vmDeletionAttempts: 5,
//         bakeImages: false,
//         exportSnapshotToBlob: false
//       }
//     },
//     buildJobsCount: 0,
//     load: 0,
//     jobStartTimeoutSeconds: 1200,
//     jobStartAttempts: 5,
//     jobStartFailureFallbackCloudName: 'electron-16-core',
//     failureAttempts: 10,
//     failureBackoffTimeSeconds: 30,
//     created: '2019-12-12T16:13:17.1087919+00:00',
//     updated: '2021-11-11T22:16:45.0855445+00:00'
//   }
