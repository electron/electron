#ifndef VirtualDisplayBridge_h
#define VirtualDisplayBridge_h

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

@interface VirtualDisplayBridge : NSObject

+ (NSInteger)addDisplay:(int)width height:(int)height;
+ (BOOL)removeDisplay:(NSInteger)displayId;

@end

@interface CGVirtualDisplay : NSObject {
  unsigned int _vendorID;
  unsigned int _productID;
  unsigned int _serialNum;
  NSString* _name;
  struct CGSize _sizeInMillimeters;
  unsigned int _maxPixelsWide;
  unsigned int _maxPixelsHigh;
  struct CGPoint _redPrimary;
  struct CGPoint _greenPrimary;
  struct CGPoint _bluePrimary;
  struct CGPoint _whitePoint;
  id _queue;
  id _terminationHandler;
  void* _client;
  unsigned int _displayID;
  unsigned int _hiDPI;
  NSArray* _modes;
  unsigned int _serverRPC_port;
  unsigned int _proxyRPC_port;
  unsigned int _clientHandler_port;
}

@property(readonly, nonatomic) NSArray* modes;
@property(readonly, nonatomic) unsigned int hiDPI;
@property(readonly, nonatomic) unsigned int displayID;
@property(readonly, nonatomic) id terminationHandler;
@property(readonly, nonatomic) id queue;
@property(readonly, nonatomic) struct CGPoint whitePoint;
@property(readonly, nonatomic) struct CGPoint bluePrimary;
@property(readonly, nonatomic) struct CGPoint greenPrimary;
@property(readonly, nonatomic) struct CGPoint redPrimary;
@property(readonly, nonatomic) unsigned int maxPixelsHigh;
@property(readonly, nonatomic) unsigned int maxPixelsWide;
@property(readonly, nonatomic) struct CGSize sizeInMillimeters;
@property(readonly, nonatomic) NSString* name;
@property(readonly, nonatomic) unsigned int serialNum;
@property(readonly, nonatomic) unsigned int productID;
@property(readonly, nonatomic) unsigned int vendorID;
- (BOOL)applySettings:(id)arg1;
- (void)dealloc;
- (id)initWithDescriptor:(id)arg1;

@end

@interface CGVirtualDisplayDescriptor : NSObject {
  unsigned int _vendorID;
  unsigned int _productID;
  unsigned int _serialNum;
  NSString* _name;
  struct CGSize _sizeInMillimeters;
  unsigned int _maxPixelsWide;
  unsigned int _maxPixelsHigh;
  struct CGPoint _redPrimary;
  struct CGPoint _greenPrimary;
  struct CGPoint _bluePrimary;
  struct CGPoint _whitePoint;
  id _queue;
  id _terminationHandler;
}

@property(retain, nonatomic) id queue;
@property(retain, nonatomic) NSString* name;
@property(nonatomic) struct CGPoint whitePoint;
@property(nonatomic) struct CGPoint bluePrimary;
@property(nonatomic) struct CGPoint greenPrimary;
@property(nonatomic) struct CGPoint redPrimary;
@property(nonatomic) unsigned int maxPixelsHigh;
@property(nonatomic) unsigned int maxPixelsWide;
@property(nonatomic) struct CGSize sizeInMillimeters;
@property(nonatomic) unsigned int serialNum;
@property(nonatomic) unsigned int productID;
@property(nonatomic) unsigned int vendorID;
- (void)dealloc;
- (id)init;
@property(copy, nonatomic) id terminationHandler;

@end

@interface CGVirtualDisplayMode : NSObject {
  unsigned int _width;
  unsigned int _height;
  double _refreshRate;
}

@property(readonly, nonatomic) double refreshRate;
@property(readonly, nonatomic) unsigned int height;
@property(readonly, nonatomic) unsigned int width;
- (id)initWithWidth:(unsigned int)arg1
             height:(unsigned int)arg2
        refreshRate:(double)arg3;

@end

@interface CGVirtualDisplaySettings : NSObject {
  NSArray* _modes;
  unsigned int _hiDPI;
}

@property(nonatomic) unsigned int hiDPI;
- (void)dealloc;
- (id)init;
@property(retain, nonatomic) NSArray* modes;

@end

#endif